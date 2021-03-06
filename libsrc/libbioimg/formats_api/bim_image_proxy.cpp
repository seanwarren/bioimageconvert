/*******************************************************************************

  Image Proxy Class - opens fomrat session and allows reading tiles and levels
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#include <cmath>
#include <cstring>

#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <bim_img_format_interface.h>
#include <xstring.h>
#include <xtypes.h>
#include <bim_metatags.h>
#include <meta_format_manager.h>

#include "bim_image_proxy.h"

using namespace bim;

//------------------------------------------------------------------------------
// ImageProxy
//------------------------------------------------------------------------------

ImageProxy::ImageProxy() {
    init();
}

ImageProxy::~ImageProxy() {
    closeFile();
    delete fm;
}

ImageProxy::ImageProxy( const std::string &fileName ) {
    init();
    openFile(fileName);
}

void ImageProxy::init() {
    fm = new MetaFormatManager();
    progress_proc = NULL;
    error_proc = NULL;
    test_abort_proc = NULL;
    external_manager = false;
}

bool ImageProxy::openFile(const std::string &fileName) {
    return fm->sessionStartRead( (bim::Filename) fileName.c_str()) == 0;
}

void ImageProxy::closeFile() { 
    if (external_manager) {
        external_manager = false;
        fm = new MetaFormatManager();
    } else {
        fm->sessionEnd();
    }
};

//------------------------------------------------------------------------------

bool ImageProxy::read(Image &img, bim::uint page) {
    return fm->sessionReadImage(img.imageBitmap(), page) == 0;
}

int ImageProxy::getImageLevel(bim::uint level) {
    fm->sessionParseMetaData(fm->sessionGetCurrentPage());
    //int levels = fm->get_metadata_tag_int(bim::IMAGE_NUM_RES_L, 0);
    xstring s = fm->get_metadata_tag(bim::IMAGE_RES_L_SCALES, "");
    std::vector<double> scales = s.splitDouble(",");

    double requested_scale = 1.0 / pow(2.0, (double)level);
    int requested_level = -1;
    for (int i = 0; i < scales.size(); ++i) {
        if ( fabs(scales[i] - requested_scale) < 0.01) {
            requested_level = i;
            break;
        }
    }
    return requested_level;
}

bool ImageProxy::readLevel(Image &img, bim::uint page, bim::uint level) {
    int requested_level = getImageLevel(level);
    if (requested_level < 0) return 1;
    return fm->sessionReadLevel(img.imageBitmap(), page, requested_level) == 0;
}

bool ImageProxy::readTile(Image &img, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level, bim::uint tile_size) {   
    bim::uint64 x1 = xid * tile_size;
    bim::uint64 x2 = xid * tile_size + tile_size - 1;
    bim::uint64 y1 = yid * tile_size;
    bim::uint64 y2 = yid * tile_size + tile_size - 1;
    return this->readRegion(img, page, x1, y1, x2, y2, level);
}

bool ImageProxy::readRegion(Image &img, bim::uint page, bim::uint64 x1, bim::uint64 y1, bim::uint64 x2, bim::uint64 y2, bim::uint level) {
    int requested_level = getImageLevel(level);
    if (requested_level < 0) return 1;

    int im_tile_sz = fm->get_metadata_tag_int(bim::TILE_NUM_X, 0);
    if (im_tile_sz < 1 || im_tile_sz != fm->get_metadata_tag_int(bim::TILE_NUM_Y, 0))
        return false;

    bim::uint64 tile_size_x = x2 - x1 + 1;
    bim::uint64 tile_size_y = y2 - y1 + 1;

    bool flat_structure = fm->get_metadata_tag(bim::IMAGE_RES_STRUCTURE, "") == bim::IMAGE_RES_STRUCTURE_FLAT;
    if (flat_structure && level>0) {
        // tiles get progressively smaller with the flat structure, adjust tile size and positions accordingly
        im_tile_sz = bim::round<bim::uint64>((double)im_tile_sz / pow(2.0, level));
        ImageInfo info = fm->sessionGetInfo();
        bim::uint64 w = bim::round<bim::uint64>((double)info.width / pow(2.0, level));
        bim::uint64 h = bim::round<bim::uint64>((double)info.height / pow(2.0, level));
        if (w<tile_size_x && h<tile_size_y)
            return fm->sessionReadLevel(img.imageBitmap(), page, level);
    }

    // if the requested block is exactly the size stored in the image, read directly
    if (tile_size_x == im_tile_sz && tile_size_y == im_tile_sz) {
        bim::uint64 xid = x1 / im_tile_sz;
        bim::uint64 yid = y1 / im_tile_sz;
        return fm->sessionReadTile(img.imageBitmap(), page, xid, yid, requested_level) == 0;
    }

    // if file tile size is different from the requested size, compose output tile from stored tiles
    double xt1 = x1 / (double)im_tile_sz;
    double xt2 = x2 / (double)im_tile_sz;
    double yt1 = y1 / (double)im_tile_sz;
    double yt2 = y2 / (double)im_tile_sz;

    int xid1 = floor(xt1);
    int nx = bim::max<int>(ceil(xt2) - xid1, 1);
    int yid1 = floor(yt1);
    int ny = bim::max<int>(ceil(yt2) - yid1, 1);

    // read all required tiles into the temp image
    Image temp;
    Image tile;
    for (bim::int64 x = 0; x < nx; ++x) {
        for (bim::int64 y = 0; y < ny; ++y) {
            if (fm->sessionReadTile(tile.imageBitmap(), page, x + xid1, y + yid1, requested_level) == 0) {
                if (temp.isEmpty())
                    temp.alloc(nx * im_tile_sz, ny * im_tile_sz, tile.samples(), tile.depth(), tile.pixelType());
                temp.setROI(x*im_tile_sz, y*im_tile_sz, tile);
            }
        } // y
    } // x

    // compute level image size
    xstring s = fm->get_metadata_tag(bim::IMAGE_RES_L_SCALES, "");
    std::vector<double> scales = s.splitDouble(",");
    float scale = scales[requested_level];
    int level_w = bim::round<double>(scale*fm->get_metadata_tag_int(bim::IMAGE_NUM_X, 0));
    int level_h = bim::round<double>(scale*fm->get_metadata_tag_int(bim::IMAGE_NUM_Y, 0));

    // extract requested ROI
    int dx = x1 - (xid1*im_tile_sz);
    int dy = y1 - (yid1*im_tile_sz);
    int szx = std::min<int>(tile_size_x, level_w - x1);
    int szy = std::min<int>(tile_size_y, level_h - y1);
    if (szx>0 && szy>0)
        img = temp.ROI(dx, dy, szx, szy);
    else
        img = Image();
    return true;
}
