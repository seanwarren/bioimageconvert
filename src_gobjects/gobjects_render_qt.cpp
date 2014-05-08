/*******************************************************************************

5D GObjects Qt rendereing

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
2008-01-01 17:02 - First creation

ver: 2

*******************************************************************************/

#include <QtCore>
#include <QtGui>

#include <cmath>
#include <algorithm>

//BioImg
#include <BioImageCore>
#include <BioImage>
#include <BioImageFormats>

#include "gobjects.h"
#include "gobjects_render_qt.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


//---------------------------------------------------------------------------------
// Utils
//---------------------------------------------------------------------------------

template<typename T>
inline T getCoordinateX( const DVertex5D<T> &v, DGObjectRenderingOptionsQt * /*opt*/ ) { 
  return v.getX();
}

template<typename T>
inline T getCoordinateY( const DVertex5D<T> &v, DGObjectRenderingOptionsQt * /*opt*/ ) { 
  return v.getY();
}

template<typename T>
inline T getCoordinateZ( const DVertex5D<T> &v, DGObjectRenderingOptionsQt * /*opt*/ ) { 
  // at this point, use T if Z is not defined
  T coord3 = v.getZ();
  //if (dim::isnan(coord3)) coord3 = v.t;
  if (dim::isnan(coord3)) coord3 = 0;
  return coord3;
}


//---------------------------------------------------------------------------------
// rendering options
//---------------------------------------------------------------------------------

void DGObjectRenderingOptionsQt::init() {
  painter = 0;
  mask_output = false;
  current_pos.resize( DGObjectRenderingOptionsQt::NumAxis );
  current_pos.fill(0);
}

QColor DGObjectRenderingOptionsQt::getGObjectColor(const DGObject &o) const {
  if (mask_output) return QColor(255,255,255);
  return DGObjectRenderingOptions::getGObjectColor(o);
}

double DGObjectRenderingOptionsQt::getGObjectLineWidth(const DGObject &o) const {
  if (mask_output) return 1.0;
  return DGObjectRenderingOptions::getGObjectLineWidth(o);
}


//---------------------------------------------------------------------------------
// Point
//---------------------------------------------------------------------------------

void draw_point( QPainter *p, const QPointF &pt, const QColor &c, const int &sz, const int &width ) {

  p->setRenderHint(QPainter::Antialiasing, true);
  QRectF r(0, 0, sz, sz);
  r.moveCenter( pt );

  p->setPen( QPen(c, width, Qt::SolidLine, Qt::RoundCap) );
  p->setBrush( QBrush(c, Qt::NoBrush) );

  // if point drawing shape is default
  p->drawEllipse(r);
  if (width>2.2) {
    r = QRectF( 0, 0, r.width()/1.414, r.height()/1.414 );
    r.moveCenter( pt );
    p->drawLine(r.topLeft(), r.bottomRight());
    p->drawLine(r.topRight(), r.bottomLeft());
  }
}

void gobject_render_point_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 1) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;

  if (!ro->isVertexVisible(o.vertices[0])) return;
  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;

  float x = getCoordinateX( o.vertices[0], ro);
  float y = getCoordinateY( o.vertices[0], ro);
  double lw = ro->getGObjectLineWidth(o);
  
  draw_point( p, QPointF(x,y), c, lw*5, lw );
}

//---------------------------------------------------------------------------------
// Polyline
//---------------------------------------------------------------------------------

void gobject_render_polyline_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {

  if (o.vertices.size() < 1) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;

  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;

  double lw = ro->getGObjectLineWidth(o);
  bool freeHand = QVariant(o.tags.value("freehand", "false")).toBool();
  QString lineStyle = o.tags.value("line_style", "solid").toLower();
  //<tag value="rectangle" name="vertex_shape" />

  p->setRenderHint(QPainter::Antialiasing, true);

  // extract all the visible vertices
  QPolygonF polygon;
  for (int i=0; i<o.vertices.size(); ++i)
    if (ro->isVertexVisible(o.vertices[i]))
      polygon << QPointF( o.vertices[i].getX(), o.vertices[i].getY() );

  // draw all the vertices
  if (!freeHand && !ro->getPolyHideVertices())
    for (int i=0; i<polygon.size(); ++i)
      draw_point( p, polygon[i], c, lw*5, lw );

  // draw polyline
  p->setBrush(QBrush( c, Qt::SolidPattern ));
  QPen pen( c, lw, Qt::SolidLine, Qt::RoundCap );
  if (lineStyle != "solid")  pen.setStyle(Qt::SolidLine);
  if (lineStyle == "dashed") pen.setStyle(Qt::DashLine);
  if (lineStyle == "dotted") pen.setStyle(Qt::DotLine);
  if (lineStyle == "dash-dotted") pen.setStyle(Qt::DashDotLine);
  if (lineStyle == "dash-dot-dotted") pen.setStyle(Qt::DashDotDotLine);
  p->setPen(pen);

  p->drawPolyline ( polygon );
}

//---------------------------------------------------------------------------------
// Polygon
//---------------------------------------------------------------------------------

void gobject_render_polygon_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 1) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;

  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;

  double lw = ro->getGObjectLineWidth(o);
  bool freeHand = QVariant(o.tags.value("freehand", "false")).toBool();
  QString lineStyle = o.tags.value("line_style", "solid").toLower();
  //<tag value="rectangle" name="vertex_shape" />

  // extract all the visible vertices
  QPolygonF polygon;
  for (int i=0; i<o.vertices.size(); ++i)
    if (ro->isVertexVisible(o.vertices[i]))
      polygon << QPointF( o.vertices[i].getX(), o.vertices[i].getY() );

  p->setRenderHint(QPainter::Antialiasing, true);

  // draw all the vertices
  if (!freeHand && !ro->getPolyHideVertices())
    for (int i=0; i<polygon.size(); ++i)
      draw_point( p, polygon[i], c, lw*5, lw );

  // draw polyline
  p->setBrush(QBrush( c, Qt::NoBrush ));
  QPen pen( c, lw, Qt::SolidLine, Qt::RoundCap );
  if (lineStyle != "solid")  pen.setStyle(Qt::SolidLine);
  if (lineStyle == "dashed") pen.setStyle(Qt::DashLine);
  if (lineStyle == "dotted") pen.setStyle(Qt::DotLine);
  if (lineStyle == "dash-dotted") pen.setStyle(Qt::DashDotLine);
  if (lineStyle == "dash-dot-dotted") pen.setStyle(Qt::DashDotDotLine);
  p->setPen(pen);

  p->drawPolygon ( polygon, Qt::WindingFill );
}

//---------------------------------------------------------------------------------
// Rectangle
//---------------------------------------------------------------------------------

void gobject_render_rectangle_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 2) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;

  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;

  p->setRenderHint(QPainter::Antialiasing, true);

  double lw = ro->getGObjectLineWidth(o);
  QString lineStyle = o.tags.value("line_style", "solid").toLower();

  float x1 = o.vertices[0].getX();
  float y1 = o.vertices[0].getY();
  float x2 = o.vertices[1].getX();
  float y2 = o.vertices[1].getY();

  QPolygonF polygon;
  if (ro->isVertexVisible(o.vertices[0])) {
    polygon << QPointF( x1, y1 );
    polygon << QPointF( x2, y1 );
  }
  if (ro->isVertexVisible(o.vertices[1])) {
    polygon << QPointF( x2, y2 );
    polygon << QPointF( x1, y2 );
  }

  p->setBrush(QBrush( c, Qt::NoBrush ));
  QPen pen( c, lw, Qt::SolidLine, Qt::RoundCap );
  if (lineStyle != "solid")  pen.setStyle(Qt::SolidLine);
  if (lineStyle == "dashed") pen.setStyle(Qt::DashLine);
  if (lineStyle == "dotted") pen.setStyle(Qt::DotLine);
  if (lineStyle == "dash-dotted") pen.setStyle(Qt::DashDotLine);
  if (lineStyle == "dash-dot-dotted") pen.setStyle(Qt::DashDotDotLine);
  p->setPen(pen);
  p->drawPolygon ( polygon, Qt::WindingFill );
}

//---------------------------------------------------------------------------------
// Ellipse
//---------------------------------------------------------------------------------

void gobject_render_ellipse_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 3) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;
  if ( !ro->isVertexVisible(o.vertices[0]) || !ro->isVertexVisible(o.vertices[1]) || !ro->isVertexVisible(o.vertices[2]) ) return;
  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;

  p->setRenderHint(QPainter::Antialiasing, true);

  double lw = ro->getGObjectLineWidth(o);
  QString lineStyle = o.tags.value("line_style", "solid").toLower();

  float x1 = o.vertices[0].x;
  float y1 = o.vertices[0].y;
  float x2 = o.vertices[1].x;
  float y2 = o.vertices[1].y;
  float x3 = o.vertices[2].x;
  float y3 = o.vertices[2].y;

  p->setBrush(QBrush( c, Qt::NoBrush ));
  QPen pen( c, lw, Qt::SolidLine, Qt::RoundCap );
  if (lineStyle != "solid")  pen.setStyle(Qt::SolidLine);
  if (lineStyle == "dashed") pen.setStyle(Qt::DashLine);
  if (lineStyle == "dotted") pen.setStyle(Qt::DotLine);
  if (lineStyle == "dash-dotted") pen.setStyle(Qt::DashDotLine);
  if (lineStyle == "dash-dot-dotted") pen.setStyle(Qt::DashDotDotLine);
  p->setPen(pen);
  
  p->drawEllipse( QPointF(x1, y1), std::max(x2, x3)-x1, std::max(y2, y3)-y1 );
}

//---------------------------------------------------------------------------------
// Circle
//---------------------------------------------------------------------------------

void gobject_render_circle_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 2) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;
  if ( !ro->isVertexVisible(o.vertices[0]) || !ro->isVertexVisible(o.vertices[1]) ) return;

  p->setRenderHint(QPainter::Antialiasing, true);

  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;
  
  double lw = ro->getGObjectLineWidth(o);
  QString lineStyle = o.tags.value("line_style", "solid").toLower();

  float x1 = o.vertices[0].x;
  float y1 = o.vertices[0].y;
  float x2 = o.vertices[1].x;
  float y2 = o.vertices[1].y;

  p->setBrush(QBrush( c, Qt::NoBrush ));
  QPen pen( c, lw, Qt::SolidLine, Qt::RoundCap );
  if (lineStyle != "solid")  pen.setStyle(Qt::SolidLine);
  if (lineStyle == "dashed") pen.setStyle(Qt::DashLine);
  if (lineStyle == "dotted") pen.setStyle(Qt::DotLine);
  if (lineStyle == "dash-dotted") pen.setStyle(Qt::DashDotLine);
  if (lineStyle == "dash-dot-dotted") pen.setStyle(Qt::DashDotDotLine);
  p->setPen(pen);
  
  double r = std::max(x2-x1, y2-y1);
  p->drawEllipse( QPointF(x1, y1), r, r );
}

//---------------------------------------------------------------------------------
// Label - small stupid fact, qt enables GL_DEPTH_TEST, so we have to rewrite
//---------------------------------------------------------------------------------

void gobject_render_label_qt( DGObjectRenderingOptions *opt, const DGObject &o ) {
  if (o.vertices.size() < 1) return;
  DGObjectRenderingOptionsQt *ro = (DGObjectRenderingOptionsQt *) opt;
  QPainter *p = ro->getPainter();
  if (!p) return;

  if (!ro->isVertexVisible(o.vertices[0])) return;
  QColor c = ro->getGObjectColor(o);
  if (c.alpha()==0) return;
  double lw = ro->getGObjectLineWidth(o);

  p->setRenderHint(QPainter::Antialiasing, true);

  float font_size = o.tags.value("font_size", "10.0").toFloat();
  QString description = o.tags.value("description", "");
  QFont font = p->font();
  font.setPixelSize(font_size);
  p->setFont(font);
  p->setPen(c);

  p->drawText( o.vertices[0].getX(), o.vertices[0].getY(), description );
}

