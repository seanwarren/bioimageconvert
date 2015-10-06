/*******************************************************************************

  Defines Image Proxy Class
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/23/2004 18:03 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_IMAGE_PROXY_H
#define BIM_IMAGE_PROXY_H

#include <bim_img_format_interface.h>
#include <bim_img_format_utils.h>
#include <tag_map.h>
#include <bim_image.h>
#include <tag_map.h>
#include <bim_image.h>
#include <meta_format_manager.h>

#include <vector>
#include <map>
#include <string>

namespace bim {

//------------------------------------------------------------------------------
// ImageProxy
//------------------------------------------------------------------------------

class ImageProxy {
public:

    ImageProxy();
    ImageProxy(MetaFormatManager *manager) { fm = manager; external_manager = true; }
    ImageProxy(const std::string &fileName);
    ~ImageProxy();

    void free()  { closeFile(); }
    void clear() { closeFile(); }

    bool isReady() const { return fm->sessionActive(); } // true if images can be used

    // I/O
    bool openFile(const std::string &fileName);
    void closeFile();

    // Operations
    bool read(Image &img, bim::uint page);
    bool readLevel(Image &img, bim::uint page, bim::uint level);
    bool readTile(Image &img, bim::uint page, bim::uint64 xid, bim::uint64 yid, bim::uint level, bim::uint tile_size);
    bool readRegion(Image &img, bim::uint page, bim::uint64 x1, bim::uint64 y1, bim::uint64 x2, bim::uint64 y2, bim::uint level);

public:
    int getImageLevel(bim::uint level);

protected:
    MetaFormatManager *fm;
    bool external_manager;

protected:
    void init();

    // callbacks
public:
    ProgressProc progress_proc;
    ErrorProc error_proc;
    TestAbortProc test_abort_proc;

protected:
    void do_progress(bim::uint64 done, bim::uint64 total, char *descr) {
        if (progress_proc) progress_proc(done, total, descr);
    }
    bool progress_abort() {
        if (!test_abort_proc) return false;
        return (test_abort_proc() != 0);
    }

};

} // bim

#endif //BIM_IMAGE_PROXY_H
