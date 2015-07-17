/*******************************************************************************

  GOjects is an application to process graphical objects XML file

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2008-01-01 17:02 - First creation

  ver: 1

*******************************************************************************/

#include <QApplication>

#include <iostream>

#include <BioImageCore>
#include <BioImage>
#include <BioImageFormats>

#include "gobjects.h"
#include "gobjects_render_qt.h"
#include "gobjects_masks.h"

//--------------------------------------------------------------------------------------
// GConf
//--------------------------------------------------------------------------------------

class GConf: public XConf {

public:
  GConf(): XConf() {}
  GConf(int argc, char** argv) { readParams( argc, argv ); }
  std::string image_input;
  std::string image_output;
  std::string gobs_input;
  std::string gobs_output;
  std::string output_format;

public:
  virtual void cureParams();

protected: 
  virtual void init();    
  virtual void processArguments();
};

void GConf::init() {
  XConf::init();
  appendArgumentDefinition( "-i", 1 );
  appendArgumentDefinition( "-ig", 1 );
  appendArgumentDefinition( "-o", 1 );
  appendArgumentDefinition( "-og", 1 );
  appendArgumentDefinition( "-t", 1 );
}

void GConf::processArguments() {
  image_input   = getValue("-i");
  image_output  = getValue("-o");
  gobs_input    = getValue("-ig");
  gobs_output   = getValue("-og");
  output_format = getValue("-t");

  /*
  file_csv    = getValue("-csv");
  process_volume = keyExists( "-vol" );
  convert_volume = keyExists( "-cnv" );

  file_stack  = getValue("-stk");
  file_stack_out  = getValue("-stko");
  append_probabilities = keyExists( "-prob" );
  channel = getValueInt("-c", 0);
  radius = getValueDouble("-r", 3.0);

  filter_by_prob = getValueDouble("-filtprob", 0.0);
  
  resize_stack = keyExists( "-resize" );
  maskout_stack = keyExists( "-maskout" );
  stat_3d = keyExists( "-stat3d" ); 
  intensity = keyExists( "-intensity" );
  prj_image = keyExists( "-prj" );
  projection = keyExists( "-projection" );

  if (file_mask.size()>0) filter_by_mask = true;
  if (file_csv.size()>0) store_csv = true;
  */
}

void GConf::cureParams() {
  //if (process_volume) process_gobjects = false;
}

//--------------------------------------------------------------------------------------
// GObjects
//--------------------------------------------------------------------------------------
/*
void walk_remove( QList<DGObject> *l, TDimImage *mask ) {
  for (int i=l->size()-1; i>=0; --i) {
    if ((*l)[i].vertices.size()>0) {
      int x = (*l)[i].vertices[0].getX();
      int y = (*l)[i].vertices[0].getY(); 
      unsigned char p = mask->pixel<unsigned char>(0, x, y);
      if (p<200) { l->removeAt(i); continue; }
    }
    walk_remove( &(*l)[i].children, mask );
  }
}

void filter_by_mask( DGObjects &gobjects, GConf &conf ) {
  TDimImage mask( conf.file_mask, 0 );
  std::cout << "Initial objects: " << gobjects.total_count() << "\n";
  walk_remove( &gobjects, &mask );
  std::cout << "Final objects: " << gobjects.total_count() << "\n";
}

void store_csv( DGObjects &gobjects, GConf &conf ) {
  QFile file( conf.file_csv.c_str() );
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
  QTextStream out(&file);
  out << gobjects.toCSVString();
}

void count_masked_volume( GConf &conf ) {
  TDimImage mask( conf.file_mask, 0 );
  DImageStack stack( conf.file_input );
  DImageStack original( conf.file_output );

  // get the pixel size from the original
  double x_um = original.get_metadata_tag_double( "pixel_resolution_x", 1.0 );
  double y_um = original.get_metadata_tag_double( "pixel_resolution_y", 1.0 );
  double z_um = original.get_metadata_tag_double( "pixel_resolution_z", 1.0 );

  // get number of planes from the mosaic
  int z = stack.length();

  // get the number of pixels in the mask
  unsigned int total = 0;
  for (unsigned int y=0; y<mask.height(); ++y) {
    unsigned char *p = mask.scanLine( 0, y );
    for (unsigned int x=0; x<mask.width(); ++x) {
      if (p[x] > 128) total++;
    } // x
  } // y

  // compute the area in pixels and microns of the masked region
  double pixel_volume_um = x_um * y_um * z_um;
  unsigned int volume = total * z;

  double volume_um = volume * pixel_volume_um;

  std::cout << "Stack size (pixels): " << stack.width() << ", " << stack.height() << ", "<< stack.length() << "\n";
  std::cout << "Stack size (um): " << stack.width()*x_um << ", " << stack.height()*y_um << ", "<< stack.length()*z_um << "\n";
  std::cout << "Image resolution (um/pixel): " << x_um << ", " << y_um << ", "<< z_um << "\n";
  std::cout << "Pixel volume (um): " << pixel_volume_um << "\n";
  std::cout << "Masked pixels: " << total << "\n";
  std::cout << "Masked volume: " << volume << "px " << volume_um << "um\n";
}

void convert_to_8( GConf &conf ) {
  DImageStack stack( conf.file_input );
  if (stack.size() > 0) {
    stack.ensureTypedDepth();
    stack.discardLut();
    stack.normalize( 8, false );
  }
  stack.toFile( conf.file_output, "TIFF" );
}

//--------------------------------------------------------------------------------------
// probabilities
//--------------------------------------------------------------------------------------

double get_probability( DImageStack *stack, double x, double y, double z, int radius_pix_xy, int radius_pix_z ) {
  int channel = 0;
  DImageStack roi = stack->deepCopy( x-radius_pix_xy, y-radius_pix_xy, z-radius_pix_z, radius_pix_xy*2, radius_pix_xy*2, radius_pix_z*2);
  DStackHistogram h( roi );
  return h[channel]->average();
}

void walk_tag( QList<DGObject> *l, DImageStack *stack, int radius_pix_xy, int radius_pix_z, double &maxp ) {
  // first tag images with probability
  for (int i=l->size()-1; i>=0; --i) {
    if ((*l)[i].vertices.size()>0) {
      double x = (*l)[i].vertices[0].getX();
      double y = (*l)[i].vertices[0].getY(); 
      double z = (*l)[i].vertices[0].getZ(); 
      double p = get_probability( stack, x, y, z, radius_pix_xy, radius_pix_z );
      if (p>maxp) maxp=p;
      (*l)[i].tags["probability"] = xstring::xprintf("%f", p).c_str(); 
    }
    walk_tag( &(*l)[i].children, stack, radius_pix_xy, radius_pix_z, maxp );
  }
}

void walk_prob( QList<DGObject> *l, DImageStack *stack, double maxp ) {
  // now normalize probability to 0-100
  for (int i=l->size()-1; i>=0; --i) {
    if ((*l)[i].vertices.size()>0) {
      double p = (*l)[i].tags["probability"].toDouble();
      p = 100.0*p/maxp;
      (*l)[i].tags["probability"] = xstring::xprintf("%f", p).c_str(); 
    }
    walk_prob( &(*l)[i].children, stack, maxp );
  }
}

void tag_by_image( DGObjects &gobjects, GConf &conf ) {
  std::cout << "Input objects: " << gobjects.total_count() << "\n";
  DImageStack stack( conf.file_stack, 0, 0, conf.channel );
  stack.normalize();
  std::cout << "Image size: " << stack.width() << "x" << stack.height() << "x" << stack.length() << "\n";
  double x_um = stack.get_metadata_tag_double( "pixel_resolution_x", 1.0 );
  //double y_um = stack.get_metadata_tag_double( "pixel_resolution_y", 1.0 );
  double z_um = stack.get_metadata_tag_double( "pixel_resolution_z", 1.0 );
  double max_probability = 0.0;
  walk_tag( &gobjects, &stack, dim::round<int>(conf.radius/x_um), dim::round<int>(conf.radius/z_um), max_probability );
  std::cout << "Maximum probability intensity: " << max_probability << "\n";
  walk_prob( &gobjects, &stack, max_probability );
}

//--------------------------------------------------------------------------------------
// filter probabilities
//--------------------------------------------------------------------------------------

void walk_filter( QList<DGObject> *l, double maxp ) {
  for (int i=l->size()-1; i>=0; --i) {
    if ((*l)[i].vertices.size()>0) {
      double p = (*l)[i].tags["probability"].toDouble();
      if (p<maxp)
        l->removeAt(i);
    }
    walk_filter( &(*l)[i].children, maxp );
  }
}

void filter_by_prob( DGObjects &gobjects, GConf &conf ) {
  std::cout << "Input objects: " << gobjects.total_count() << "\n";
  walk_filter( &gobjects, conf.filter_by_prob );
  std::cout << "Output objects: " << gobjects.total_count() << "\n";
}

//--------------------------------------------------------------------------------------
// stacks
//--------------------------------------------------------------------------------------

void resize_stack( GConf &conf ) {
  DImageStack stack( conf.file_stack );
  std::cout << "Stack size: " << stack.width() << "x" << stack.height() << "x" << stack.length() << "\n";

  std::vector<int> map;
  map.push_back(1);
  map.push_back(2);
  stack.remapChannels(map);
  stack.normalize();

  stack.resize( 1024, 1024, 64, TDimImage::szBiCubic, false);
  std::cout << "New size: " << stack.width() << "x" << stack.height() << "x" << stack.length() << "\n";

  stack.toFile( conf.file_stack_out, "tiff" );
}


//--------------------------------------------------------------------------------------
// stacks
//--------------------------------------------------------------------------------------

void mask_out_stack( GConf &conf ) {
  DImageStack stack( conf.file_stack );
  std::cout << "Stack size: " << stack.width() << "x" << stack.height() << "x" << stack.length() << "\n";
  stack.normalize();

  DImageStack mask( conf.file_mask );
  std::cout << "Mask size: " << mask.width() << "x" << mask.height() << "x" << mask.length() << "\n";

  if ( stack.width()!=mask.width() || stack.height()!=mask.height() || stack.length()!=mask.length() ) {
    std::cout << "Image and mask sizes must match!\n";
    return;
  }

  // set the mask to have same number of channels
  std::vector<int> map;
  for (int i=0; i<stack.samples(); ++i) map.push_back(0);
  //stack.remapChannels(map);

  for (int i=0; i<stack.length(); ++i) {
    mask[i]->operationThershold(10);
    mask[i]->remapChannels(map);
    stack[i]->pixelArithmeticMin( *mask[i] );
    std::cout << ".";
  }

  stack.toFile( conf.file_stack_out, "ome-tiff" );
}

//--------------------------------------------------------------------------------------
// stats
//--------------------------------------------------------------------------------------

void statistics( GConf &conf ) {

  DGObjects gobjects( conf.file_input.c_str() );
  TDimImage mask( conf.file_mask, 0 );
  DImageStack stack( conf.file_input );
  DImageStack original( conf.file_output );

  // get the pixel size from the original
  double x_um = original.get_metadata_tag_double( "pixel_resolution_x", 1.0 );
  double y_um = original.get_metadata_tag_double( "pixel_resolution_y", 1.0 );
  double z_um = original.get_metadata_tag_double( "pixel_resolution_z", 1.0 );

  // get number of planes from the mosaic
  int z = stack.length();

  // get the number of pixels in the mask
  unsigned int total = 0;
  for (unsigned int y=0; y<mask.height(); ++y) {
    unsigned char *p = mask.scanLine( 0, y );
    for (unsigned int x=0; x<mask.width(); ++x) {
      if (p[x] > 128) total++;
    } // x
  } // y

  // compute the area in pixels and microns of the masked region
  unsigned int volume = total * z;
  double pixel_volume_um = x_um * y_um * z_um;
  double volume_um = volume * pixel_volume_um;

  std::cout << "Stack size (pixels): " << stack.width() << ", " << stack.height() << ", "<< stack.length() << "\n";
  std::cout << "Stack size (um): " << stack.width()*x_um << ", " << stack.height()*y_um << ", "<< stack.length()*z_um << "\n";
  std::cout << "Image resolution (um/pixel): " << x_um << ", " << y_um << ", "<< z_um << "\n";
  std::cout << "Pixel volume (um): " << pixel_volume_um << "\n";
  std::cout << "Masked pixels: " << total << "\n";
  std::cout << "Masked volume: " << volume << "px " << volume_um << "um\n";
}

void statistics3d( GConf &conf ) {

  // load reduced version of the original image, we only need it for metadata
  DImageStack stack( conf.file_stack, 128, 128, 0 );
  std::cout << "Stack file: " << conf.file_stack << "\n";
  std::cout << "Stack size: " << stack.width() << "x" << stack.height() << "x" << stack.length() << "\n";

  // get the voxel size from the original
  double x_um = stack.get_metadata_tag_double( "pixel_resolution_x", 1.0 );
  double y_um = stack.get_metadata_tag_double( "pixel_resolution_y", 1.0 );
  double z_um = stack.get_metadata_tag_double( "pixel_resolution_z", 1.0 );
  
  stack.free();



  DImageStack mask( conf.file_mask );
  std::cout << "Mask file: " << conf.file_mask << "\n";
  std::cout << "Mask size: " << mask.width() << "x" << mask.height() << "x" << mask.length() << "\n";

  // get the number of voxels in the mask
  unsigned int total = 0;
  for (int i=0; i<mask.length(); ++i) {
    for (unsigned int y=0; y<mask[i]->height(); ++y) {
      unsigned char *p = mask[i]->scanLine( 0, y );
      for (unsigned int x=0; x<mask[i]->width(); ++x) {
        if (p[x] > 128) total++;
      } // x
    } // y
    std::cout << ".";
  }
  std::cout << "\n";


  // compute the area in voxels and microns of the masked region
  double voxel_volume_um = x_um * y_um * z_um;
  double volume_um = total * voxel_volume_um;

  std::cout << "Stack size (pixels): " << mask.width() << ", " << mask.height() << ", "<< mask.length() << "\n";
  std::cout << "Stack size (um): " << mask.width()*x_um << ", " << mask.height()*y_um << ", "<< mask.length()*z_um << "\n";
  std::cout << "Image resolution (um/pixel): " << x_um << ", " << y_um << ", "<< z_um << "\n";
  std::cout << "Voxel volume (um^3): " << voxel_volume_um << "\n";
  std::cout << "Masked volume: " << total << " vx, " << volume_um << " um^3\n";

  if (conf.file_input.size()<=0) return;

  DGObjects gobjects( conf.file_input.c_str() );
  double goc = gobjects.total_count();
  double um_gob = volume_um / goc;
  double gob_um = goc / volume_um;

  std::cout << "Nucl: " << goc << ", ";
  std::cout << gob_um << " nucl/um^3, ";
  std::cout << um_gob << " um^3/nucl\n";
}


//-i "G:\_kosik_cdk5\original\FV1000-2009-01-15 18.03.55-0031.oib" -m "G:\_kosik_cdk5\original\FV1000-2009-01-15 18.03.55-0031.MASK.png" -intensity
void statistics_intensity ( GConf &conf ) {

  DImageStack stack( conf.file_input );

  if (conf.prj_image) {
    TDimImage prj = stack.pixelArithmeticMax();
    stack.clear();
    stack.append( prj );
  }

  TDimImage mask;
  mask.( conf.file_mask, 0 );

  DStackHistogram h( stack, &mask );
  
  std::cout << "File: " << conf.file_input << "\n";
  std::cout << "File: " << conf.file_mask << "\n";

  for (int c=0; c<stack.samples(); ++c) {

    // get the pixel size from the original
    xstring chan_key = xstring::xprintf("channel_%d_name", c);
    xstring chan_def = xstring::xprintf("Channel %d", c);
    std::string chan_name = stack.get_metadata_tag( chan_key, chan_def );

    std::cout << chan_name << " average: " << h[c]->average() << "\n";
    std::cout << chan_name << " std: " << h[c]->std() << "\n";
    std::cout << chan_name << " max: " << h[c]->max_value() << "\n";
    std::cout << chan_name << " min: " << h[c]->min_value() << "\n";
  }
  std::cout << "done\n";
}

//-i "G:\_kosik_cdk5\original\FV1000-2009-01-15 18.03.55-0031.oib" -o "G:\_kosik_cdk5\original\FV1000-2009-01-15 18.03.55-0031.PRJ.tif" -projection
void project_stack( GConf &conf ) {

  DImageStack stack( conf.file_input );
  TDimImage prj = stack.pixelArithmeticMax();
  prj.toFile(conf.file_output, "TIFF");
}
*/

//--------------------------------------------------------------------------------------
// render gobs
//--------------------------------------------------------------------------------------

void render_gobs ( GConf &conf ) {

  DGObjects gobjects(conf.gobs_input.c_str());
  gobjects.setRenderProc( "point", gobject_render_point_qt );
  gobjects.setRenderProc( "polyline", gobject_render_polyline_qt );
  gobjects.setRenderProc( "polygon", gobject_render_polygon_qt );
  gobjects.setRenderProc( "rectangle", gobject_render_rectangle_qt );
  gobjects.setRenderProc( "square", gobject_render_rectangle_qt );
  gobjects.setRenderProc( "ellipse", gobject_render_ellipse_qt );
  gobjects.setRenderProc( "circle", gobject_render_circle_qt );
  gobjects.setRenderProc( "label", gobject_render_label_qt );

  TMetaFormatManager fm, ofm;
  TDimImage img;

  if (ofm.isFormatSupportsWMP(conf.output_format.c_str())==false) return;
  if (fm.sessionStartRead(conf.image_input.c_str())!=0) return;
  if (ofm.sessionStartWrite(conf.image_output.c_str(), conf.output_format.c_str())!=0) return;


  QString f = conf.image_output.c_str();

  int pages = fm.sessionGetNumberOfPages();
  fm.sessionReadImage( img.imageBitmap(), 0);
  fm.sessionParseMetaData(0);
  img.set_metadata(fm.get_metadata());
  ofm.sessionWriteSetMetadata( img.get_metadata() );

  int num_z = img.get_metadata_tag_int( bim::IMAGE_NUM_Z, 1 );
  int num_t = img.get_metadata_tag_int( bim::IMAGE_NUM_T, 1 );
  int cur_t = 0;
  int cur_z = 0;

  DGObjectRenderingOptionsQt opt;
  //opt.setColor(QColor(255,0,0));
  //opt.setColorOverride(true);
  //opt.setWidthMultiplier(2.0);
  opt.setPolyHideVertices(false);

  for (int page=0; page<pages; ++page) {
      // load page image, needs new clear image due to memory sharing
      if (page>0)
      if (fm.sessionReadImage( img.imageBitmap(), page ) != 0) break;

      QImage plane = img.toQImage();
      QPainter p(&plane);
      
      opt.setPainter(&p);
      opt.setCurrentZ(cur_z); 
      opt.setCurrentT(cur_t);
      gobjects.render(&opt);

      TDimImage oi(plane);
      ofm.sessionWriteImage( oi.imageBitmap(), page );

      ++cur_z;
      if (cur_z>=num_z) { ++cur_t; cur_z=0; }
  }
  fm.sessionEnd();
  ofm.sessionEnd();
}



//--------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  
  GConf conf( argc, argv );
  render_gobs (conf);

/*
  if (conf.resize_stack) {
    resize_stack( conf );
    return 0;
  }

  if (conf.maskout_stack) {
    mask_out_stack( conf );
    return 0;
  }

  if (conf.process_volume) {
    count_masked_volume( conf );
    return 0;
  }

  if (conf.convert_volume) {
    convert_to_8( conf );
    return 0;
  }

  if (conf.stat_3d) {
    statistics3d( conf );
    return 0;
  }

  if (conf.intensity) {
    statistics_intensity( conf );
    return 0;
  }

  if (conf.projection) {
    project_stack( conf );
    return 0;
  }

  
  DGObjects gobjects( conf.file_input.c_str() );

  if (conf.filter_by_mask) 
    filter_by_mask( gobjects, conf );

  if (conf.append_probabilities) 
    tag_by_image( gobjects, conf );

  if (conf.store_csv) 
    store_csv( gobjects, conf );

  if (conf.filter_by_prob>0.0) 
    filter_by_prob( gobjects, conf );

  if (conf.file_output.size()>0) 
    gobjects.toFile( conf.file_output.c_str() );

*/

  return 0;
}
