/*******************************************************************************

GObject Masks

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
2008-01-01 17:02 - First creation

ver: 2

*******************************************************************************/

#include <cmath>

#include <QtCore>
#include <QtGui>

#include <BioImageCore>
#include <BioImage>
#include <BioImageFormats>

#include "gobjects_masks.h"
#include "gobjects.h"
#include "gobjects_render_qt.h"

#ifdef BIM_USE_PROGRESS
#include <d_progress_widget.h>
#endif

//---------------------------------------------------------------------------------
// Masks into files
//---------------------------------------------------------------------------------

void renderGObjectsToImage( const QString &name, const QString &format, 
                       DGObjects &gobjects, int w, int h, int l,
                       DProgressWidget *progress, 
                       const DGObjectRenderingOptions &rend_opts,
                       const QString &args ) {
  
  #ifdef BIM_USE_PROGRESS
  if (progress) progress->startNow(true);
  #endif

  gobjects.setRenderProc( "point", gobject_render_point_qt );
  gobjects.setRenderProc( "polyline", gobject_render_polyline_qt );
  gobjects.setRenderProc( "polygon", gobject_render_polygon_qt );
  //gobjects.setRenderProc( "rectangle", gobject_render_rectangle_qt );
  //gobjects.setRenderProc( "square", gobject_render_rectangle_qt );
  //gobjects.setRenderProc( "ellipse", gobject_render_ellipse_qt );
  //gobjects.setRenderProc( "circle", gobject_render_circle_qt );
  //gobjects.setRenderProc( "label", gobject_render_label_qt );

  QImage plane( w, h, QImage::Format_ARGB32 );
  QPainter p(&plane);

  DGObjectRenderingOptionsQt opt;
  //= rend_opts;
  opt.setPainter(&p);
  if (args.contains("mask-mode")) opt.setMaskOutput(true);

  if (!args.contains("interpolate-empty")) {
    TMetaFormatManager fm;
    if (fm.isFormatSupportsWMP( format.toStdString().c_str() ) == false) return;
    if (fm.sessionStartWrite(name.toStdString().c_str(),  format.toStdString().c_str()) == 0) {
      for (int z=0; z<l; ++z) {
        plane.fill(0);
        opt.setCurrentZ(z);
        gobjects.render(&opt);

        TDimImage img;
        img.fromQImage( plane );
        if (opt.getMaskOutput()) img.extractChannel(0);

        fm.sessionWriteImage( img.imageBitmap(), z );
        #ifdef BIM_USE_PROGRESS
        if (progress) progress->doProgress( "Creating gobject mask", z, l-1 );
        #endif
      } // for z
    }
    fm.sessionEnd(); 
  } // if no interpolation

  if (args.contains("interpolate-empty")) {
    DImageStack stk;
    // create stack first
    for (int z=0; z<l; ++z) {
      plane.fill(0);

      opt.setCurrentZ(z);
      gobjects.render(&opt);

      TDimImage img;
      img.fromQImage( plane );
      if (opt.getMaskOutput()) img.extractChannel(0);

      stk.append(img);
      #ifdef BIM_USE_PROGRESS
      if (progress) progress->doProgress( "Creating gobject mask", z, l-1 );
      #endif
    } // for z
   
    // now interpolate all empty fields
    std::vector<int> empty;
    for (int z=l-1; z>=0; --z) {
      DImageHistogram h( *stk[z] );
      
      double m = h[0]->num_unique();
      for (int c=1; c<h.channels(); ++c)
        m = std::max<int>(m, h[c]->num_unique());

      if (m<2) {
        empty.push_back(z);
      } else {
        if (empty.size()>0) {
          int beg = std::max<int>(0, empty.back()-1);
          int end = std::min<int>(empty.front()+1, l-1);
          
          DImageStack stktmp;
          stktmp.append(stk[beg]->deepCopy());
          stktmp.append(stk[end]->deepCopy()); 
          stktmp.resize(0, 0, end-beg+1, TDimImage::szBiCubic);

          for (int p=0; p<stktmp.size(); ++p) 
            *stk[beg+p] = *stktmp[p];

          empty.clear();
        }
      }
      #ifdef BIM_USE_PROGRESS
      if (progress) progress->doProgress( "Interpolating gobject mask", l-z, l );
      #endif
    } // for z

    stk.toFile(name.toStdString().c_str(),  format.toStdString().c_str());
  } // if interpolation
  #ifdef BIM_USE_PROGRESS
  if (progress) progress->stop();
  #endif
}


