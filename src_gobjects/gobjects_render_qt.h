/*******************************************************************************

5D GObjects Qt rendereing

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
2008-01-01 17:02 - First creation

ver: 1

*******************************************************************************/

#ifndef DIM_GOBJECTS_RENDER_QT_H
#define DIM_GOBJECTS_RENDER_QT_H

#include <QObject>
#include <QColor>

#include "gobjects.h"

class DGObject;
class QWidget;

//---------------------------------------------------------------------------------
// rendering functions
//---------------------------------------------------------------------------------

void gobject_render_point_qt     ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_polyline_qt  ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_polygon_qt   ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_rectangle_qt ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_ellipse_qt   ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_circle_qt    ( DGObjectRenderingOptions *, const DGObject &o );
void gobject_render_label_qt     ( DGObjectRenderingOptions *, const DGObject &o );

//---------------------------------------------------------------------------------
// rendering options
//---------------------------------------------------------------------------------

class DGObjectRenderingOptionsQt: public DGObjectRenderingOptions {
  Q_OBJECT

public:
  enum Coordinate { NumAxis=2, Z=0, T=1 };

public:
  DGObjectRenderingOptionsQt(): DGObjectRenderingOptions() { init(); };
  DGObjectRenderingOptionsQt(QPainter *v): DGObjectRenderingOptions() { init(); setPainter(v); }

public:

  template <typename T>
  bool isVertexVisible(const DVertex5D<T> &v) const;

  QColor getGObjectColor(const DGObject &o) const;
  double getGObjectLineWidth(const DGObject &o) const;

public:
  inline double    getCurrentZ()    const { return current_pos[DGObjectRenderingOptionsQt::Z]; }
  inline double    getCurrentT()    const { return current_pos[DGObjectRenderingOptionsQt::T]; }
  inline bool      getMaskOutput()  const { return mask_output; }
  inline QPainter* getPainter()     const { return painter; }
  
public slots:
  inline void setPainter(QPainter *v)     { painter = v; }
  inline void setMaskOutput(const bool v) { mask_output = v; emit needsRendering(); }

  inline void setCurrentZ(const double v) { current_pos[DGObjectRenderingOptionsQt::Z]=v; emit needsRendering(); }
  inline void setCurrentT(const double v) { current_pos[DGObjectRenderingOptionsQt::T]=v; emit needsRendering(); }
  inline void advanceCurrentZ()           { current_pos[DGObjectRenderingOptionsQt::Z]+=1; emit needsRendering(); }
  inline void advanceCurrentT()           { current_pos[DGObjectRenderingOptionsQt::T]+=1; emit needsRendering(); }

protected:
  void init();
  QPainter *painter;
  QVector<double> current_pos;
  bool mask_output;

};

template <typename Tv>
bool DGObjectRenderingOptionsQt::isVertexVisible(const DVertex5D<Tv> &v) const {
  return (!v.hasZ() || fabs(v.getZ() - current_pos[DGObjectRenderingOptionsQt::Z]) < 0.5) &&
         (!v.hasT() || fabs(v.getT() - current_pos[DGObjectRenderingOptionsQt::T]) < 0.5);
}


#endif // DIM_GOBJECTS_RENDER_QT_H
