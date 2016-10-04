/*******************************************************************************

5D GObjects definitions

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
  2008-01-01 17:02 - First creation
  2010-07-08 21:56 - Lists hold references for speed, added visibility

ver: 2

*******************************************************************************/

#ifndef DIM_GOBJECTS_H
#define DIM_GOBJECTS_H

#include <limits>

#include <QtCore>
#include <QtGui>

#include <BioImageCore>

class QDomDocument;
class QDomElement;
class QWidget;

class DGObject;
class DGObjects;
class TDimImage;

//------------------------------------------------------------------------------------
// template for a voxel element, it has coordinate and color associated
//------------------------------------------------------------------------------------

template <typename Tv> 
class DVertex5D {
public:
  DVertex5D ( Tv vx = std::numeric_limits<Tv>::quiet_NaN(), Tv vy = std::numeric_limits<Tv>::quiet_NaN(), 
              Tv vz = std::numeric_limits<Tv>::quiet_NaN(), Tv vt = std::numeric_limits<Tv>::quiet_NaN(), 
              Tv vc = std::numeric_limits<Tv>::quiet_NaN(), int vi = -1 ) {
    x = vx; y = vy; z = vz; t = vt; c = vc; i = vi;
  }

  DVertex5D ( const QDomElement &gobject ) { fromXML( gobject ); }

  inline void setX( Tv v ) { x = v; }
  inline void setY( Tv v ) { y = v; }
  inline void setZ( Tv v ) { z = v; }
  inline void setT( Tv v ) { t = v; }
  inline void setC( Tv v ) { c = v; }
  inline void setIndex( int v ) { i = v; }
  inline const Tv getX() const { return x; }
  inline const Tv getY() const { return y; }
  inline const Tv getZ() const { return z; }
  inline const Tv getT() const { return t; }
  inline const Tv getC() const { return c; }
  inline const int getIndex() const { return i; }

  inline bool hasX() const { return !dim::isnan(x); }
  inline bool hasY() const { return !dim::isnan(y); }
  inline bool hasZ() const { return !dim::isnan(z); }
  inline bool hasT() const { return !dim::isnan(t); }
  inline bool hasC() const { return !dim::isnan(c); }
  inline bool hasIndex() const { return i != -1; }


  inline bool operator<(const DVertex5D<Tv>& o)  const { return i < o.i; }
  inline bool operator<=(const DVertex5D<Tv>& o) const { return i <= o.i; }
  inline bool operator>(const DVertex5D<Tv>& o)  const { return i > o.i; }
  inline bool operator>=(const DVertex5D<Tv>& o) const { return i >= o.i; }
  inline bool operator==(const DVertex5D<Tv>& o) const { return i == o.i; }
  inline bool operator!=(const DVertex5D<Tv>& o) const { return i != o.i; }

  inline DVertex5D<Tv>& operator+=(const DVertex5D<Tv>& o) { 
    this->x += o.x; this->y += o.y; this->z += o.z; this->t += o.t; this->c += o.c; return *this; }
  inline DVertex5D<Tv>& operator-=(const DVertex5D<Tv>& o) { 
    this->x -= o.x; this->y -= o.y; this->z -= o.z; this->t -= o.t; this->c -= o.c; return *this; }
  inline DVertex5D<Tv>& operator*=(const DVertex5D<Tv>& o) { 
    this->x *= o.x; this->y *= o.y; this->z *= o.z; this->t *= o.t; this->c *= o.c; return *this; }
  inline DVertex5D<Tv>& operator/=(const DVertex5D<Tv>& o) { 
    this->x /= o.x; this->y /= o.y; this->z /= o.z; this->t /= o.t; this->c /= o.c; return *this; }

  virtual int fromXML( const QDomElement &gobject );
  virtual int toXML( QDomDocument *doc, QDomElement *gobject ) const;

  virtual QString toCSVString() const;

public:
  // vertex model
  Tv x, y, z, t, c;
  int i;
};

//------------------------------------------------------------------------------------
// DGObject
//------------------------------------------------------------------------------------

class DGObjectRenderingOptions;
typedef void (*DGObjectRenderProc)( DGObjectRenderingOptions *, const DGObject & );

class DGObject {
public:

  DGObject( const QString &_type = "gobject", const QString &_name = "gobject", DGObject *_parent=0 ) {
    render_proc = 0;
    setType( _type );
    setName( _name );
    parent=_parent;
    visible=true;
  }

  DGObject( const QDomElement &gobject, DGObject *_parent=0 ) {
    render_proc = 0;
    fromXML( gobject );
    parent=_parent;
    visible=true;
  }

  ~DGObject() { }

  inline QString getType() const { return type; }
  void setType( const QString &v ) {
    type = v.toLower();
  }

  inline QString getName() const { return name; }
  void setName( const QString &v ) {
    name = v.toLower();
  }

  DGObject *getParent() const { return parent; }
  inline bool isVisible() const { return visible; }
  void setVisible( bool v ) { visible = v; }

  bool hasVertices() const { return (vertices.size() > 0); }

  bool isComplex() const { return ( children.size()>0 );  }
  bool isGrapicalPrimitive() const { return ( vertices.size()>0 ); }

  virtual int fromXML( const QDomElement &gobject );
  virtual int toXML( QDomDocument *doc, QDomElement *gobject ) const;

  virtual QString toCSVString() const;

  void setRenderProc( const QString &type, DGObjectRenderProc new_render_proc );
  virtual void render( DGObjectRenderingOptions * ) const;

  inline DGObject& operator+=(const DVertex5D<float>& o);
  inline DGObject& operator-=(const DVertex5D<float>& o);
  inline DGObject& operator*=(const DVertex5D<float>& o);
  inline DGObject& operator/=(const DVertex5D<float>& o);

  friend class DGObjects;
  friend class DGObjectRenderingOptions;

public:
  // gobject model description
  QString type;
  QString name;
  QList< DVertex5D<float> > vertices;
  QHash<QString, QString> tags;
  QList< QSharedPointer<DGObject> > children;

public:
  // additional methods
  DGObjectRenderProc render_proc;
  DGObject *parent;
  bool visible;

};

//------------------------------------------------------------------------------------
// DGObjects
//------------------------------------------------------------------------------------

class DGObjects : public QList< QSharedPointer<DGObject> > {
public:
  DGObjects(): QList< QSharedPointer<DGObject> >() {}
  DGObjects(const QString &f): QList< QSharedPointer<DGObject> >() { fromFile(f); }

  inline DGObjects& operator+=(const DVertex5D<float>& o);
  inline DGObjects& operator-=(const DVertex5D<float>& o);
  inline DGObjects& operator*=(const DVertex5D<float>& o);
  inline DGObjects& operator/=(const DVertex5D<float>& o);

  unsigned int total_count();
  QSet<QString> unique_type_names();
  QSet<QString> unique_tag_names();

public: // I/O

  int fromFile( const QString & );
  int toFile( const QString & ) const;

  int fromString( const QString & );
  QString toString() const;

  QString toCSVString();

public: // operations

  // the function F supposed to be something like: func( DGObject *o, P params );
  // see the use in the total_count
  template< typename F, typename P >
  void walker( F func, P params );

public: // rendering

  void render( DGObjectRenderingOptions * ) const;
  void paint(QPainter *p) const;
  void paint(TDimImage *img) const;

  void setRenderProc( const QString &type, DGObjectRenderProc new_render_proc );

  void insertDefaultRenderProc( const QString &_type, DGObjectRenderProc new_render_proc ) {
    render_procs.insert( _type, new_render_proc );
  }
  void clearDefaultRenderProcs() { render_procs.clear(); }

protected:
  QHash<QString, DGObjectRenderProc> render_procs;

  int parse( QDomDocument *dom );

private:
  template< typename F, typename P >
  void walk_gobjects( QList< QSharedPointer<DGObject> > *l, F func, P params );

};



//---------------------------------------------------------------------------------
// rendering options
//---------------------------------------------------------------------------------

class DGObjectRenderingOptions: public QObject {
  Q_OBJECT

public:
  DGObjectRenderingOptions();

  // used with the tree which can hide/show objects
  bool   isGObjectVisible(const DGObject *o) const;

  // used in rendering to determine if object is visible in the current rendering state
  // e.g. in OpenGL it is used to see if the vertex is in the current T point
  // e.g. in Qt render it is used to see if the vertex is in the current Z and T image
  template <typename T>
  bool isVertexVisible(const DVertex5D<T> &v) const { return true; }

public:
  QColor getGObjectColor(const DGObject &o) const;
  double getGObjectLineWidth(const DGObject &o) const;

  inline int           getHideBelowProbability()   const { return probability_hide_below; }
  inline double        getWidthMultiplier()        const { return size_multiplier; }
  inline unsigned char getTransparency()           const { return transparency; }
  inline bool          getTransparentProbability() const { return probability_use_transparency; }
  inline bool          getColorOverride()          const { return color_override; }
  inline QColor        getColor()                  const { return color; }
  inline bool          getColorCodeProb()          const { return probability_color_code; }
  inline QColor        getProbColorHigh()          const { return probability_color_high; }
  inline QColor        getProbColorLow()           const { return probability_color_low; }
  inline bool          getPolyHideVertices()       const { return poly_hide_vertices; }
  inline bool          getUseDepth()               const { return use_depth; }
  inline QString       getProbTagName()            const { return probability_tag_name; }
  inline QSet<QString> getInludeTypes()            const { return include_types; }
  inline QSet<QString> getExludeTypes()            const { return exclude_types; }

public slots:
  inline void setWidthMultiplier(double v)      { size_multiplier = v; emit needsRendering(); }
  inline void setWidthMultiplier(int v)         { setWidthMultiplier((double) v); }
  inline void setHideBelowProbability(int v)    { probability_hide_below = v; emit needsRendering(); }
  inline void setTransparency(unsigned char v)  { transparency = v; emit needsRendering(); }
  inline void setTransparency(int v)            { setTransparency( (unsigned char) v); }
  inline void setColor(const QColor &v)         { color = v; emit needsRendering(); }
  inline void setColorOverride(bool v)          { color_override = v; emit needsRendering(); }
  inline void setTransparentProbability(bool v) { probability_use_transparency = v; emit needsRendering(); }
  inline void setTransparentProbability(int v)  { setTransparentProbability((bool) v); }
  inline void setColorCodeProb(bool v)          { probability_color_code = v; emit needsRendering(); }
  inline void setProbColorHigh(const QColor &v) { probability_color_high = v; emit needsRendering(); }
  inline void setProbColorLow(const QColor &v)  { probability_color_low = v; emit needsRendering(); }
  inline void setPolyHideVertices(bool v)       { poly_hide_vertices = v; emit needsRendering(); }
  inline void setUseDepth(bool v)               { use_depth = v; emit needsRendering(); }
  inline void setProbTagName(const QString &v)  { probability_tag_name = v; emit needsRendering(); }
  inline void setInludeTypes(const QSet<QString> &v)  { include_types = v; emit needsRendering(); }
  inline void setExludeTypes(const QSet<QString> &v)  { exclude_types = v; emit needsRendering(); }

signals:
  void needsRendering();

protected:
  double   size_multiplier;
  unsigned char transparency;
  bool     color_override;
  QColor   color;
  bool     use_depth;

  bool   poly_hide_vertices;

  int    probability_hide_below;
  bool   probability_use_transparency;

  bool   probability_color_code;
  QColor probability_color_high;
  QColor probability_color_low;
  QString probability_tag_name;

  QSet<QString> include_types;
  QSet<QString> exclude_types;
};


#endif // DIM_GOBJECTS_H
