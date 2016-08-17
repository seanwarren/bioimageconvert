/*******************************************************************************

5D GObjects definitions

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
  2008-01-01 17:02 - First creation
  2010-07-08 21:56 - Lists hold references for speed, added visibility

ver: 2

*******************************************************************************/

#include "gobjects.h"

#include <cmath>

#include <QtCore>
#include <QtGui>
#include <QtXml>

//------------------------------------------------------------------------------------
// DVertex5D
//------------------------------------------------------------------------------------

template <typename Tv> 
int DVertex5D<Tv>::fromXML( const QDomElement &vertex_node ) {

  x = std::numeric_limits<Tv>::quiet_NaN();
  y = std::numeric_limits<Tv>::quiet_NaN();
  z = std::numeric_limits<Tv>::quiet_NaN();
  t = std::numeric_limits<Tv>::quiet_NaN();
  c = std::numeric_limits<Tv>::quiet_NaN();
  i = -1;

  if ( !vertex_node.isNull() ) {
    if (vertex_node.hasAttribute("x"))     x = vertex_node.attribute("x").toFloat(); 
    if (vertex_node.hasAttribute("y"))     y = vertex_node.attribute("y").toFloat();
    if (vertex_node.hasAttribute("z"))     z = vertex_node.attribute("z").toFloat();
    if (vertex_node.hasAttribute("t"))     t = vertex_node.attribute("t").toFloat();
    if (vertex_node.hasAttribute("c"))     c = vertex_node.attribute("c").toFloat();
    if (vertex_node.hasAttribute("index")) i = vertex_node.attribute("index").toInt();
  } 

  return 0;
}

template <typename Tv> 
int DVertex5D<Tv>::toXML( QDomDocument *doc, QDomElement *gobject ) const {
  QDomElement o = doc->createElement("vertex");
  if (hasX()) o.setAttribute( "x", getX() );
  if (hasY()) o.setAttribute( "y", getY() );
  if (hasZ()) o.setAttribute( "z", getZ() );
  if (hasT()) o.setAttribute( "t", getT() );
  if (hasC()) o.setAttribute( "c", getC() );
  if (hasIndex()) o.setAttribute( "index", getIndex() );
  gobject->appendChild(o);
  return 0;
}

template <typename Tv> 
QString DVertex5D<Tv>::toCSVString() const {
  QString s = " ";
  if (hasX()) s += QString("%1").arg(getX());
  s += ",";
  if (hasY()) s += QString("%1").arg(getY());
  s += ",";
  if (hasZ()) s += QString("%1").arg(getZ());
  s += ",";
  if (hasT()) s += QString("%1").arg(getT()); 
  s += ",";
  if (hasC()) s += QString("%1").arg(getC());
  s += ",";
  if (hasIndex()) s += QString("%1").arg(getIndex());
  s += "\n";
  return s; 
}


//------------------------------------------------------------------------------------
// DGObject
//------------------------------------------------------------------------------------

void DGObject::setRenderProc( const QString &_type, DGObjectRenderProc new_render_proc ) {
  if (_type.toLower() == type)
    render_proc = new_render_proc;
  for (int i=0; i<children.size(); ++i) {
    children.at(i)->setRenderProc( _type, new_render_proc );
  }
}

void DGObject::render( DGObjectRenderingOptions *opt ) const {
  if (!this->visible) return;
  if (opt && !opt->isGObjectVisible(this)) return;

  for (int i=0; i<children.size(); ++i) {
    children.at(i)->render( opt );
  }
  if (vertices.size() <= 0) return;
  if (render_proc) render_proc( opt, *this );
}

int DGObject::fromXML( const QDomElement &gobject ) {

  if ( gobject.hasAttribute("name") ) name = gobject.attribute("name");
  if ( gobject.hasAttribute("type") ) type = gobject.attribute("type");

  //-------------------------------------------------------------------
  // read vertices
  vertices.clear();
  QDomElement vertex_node = gobject.firstChildElement("vertex");
  while ( !vertex_node.isNull() ) {
    DVertex5D<float> vertex( vertex_node );
    vertices << vertex;
    vertex_node = vertex_node.nextSiblingElement("vertex");
  } // while

  // sort vertices by index
  qSort( vertices.begin(), vertices.end(), qLess< DVertex5D<float> >() );


  //-------------------------------------------------------------------
  // read tags
  tags.clear();

  QDomElement tag_node = gobject.firstChildElement("tag");
  while (!tag_node.isNull()) {
    QString tag_name, tag_value;

    if (tag_node.hasAttribute("name")) tag_name = tag_node.attribute("name");
    if (tag_node.hasAttribute("value")) 
      tag_value = tag_node.attribute("value");
    else
      tag_value = tag_node.firstChildElement("value").text();

    tags.insert( tag_name, tag_value );
    tag_node = tag_node.nextSiblingElement("tag");
  } // while


  //-------------------------------------------------------------------
  // read children
  children.clear();

  QDomElement gobject_node = gobject.firstChildElement("gobject");
  while ( !gobject_node.isNull() ) {
    children << QSharedPointer<DGObject>( new DGObject(gobject_node, this) );
    gobject_node = gobject_node.nextSiblingElement("gobject");
  } // while

  return 0;
}

int DGObject::toXML( QDomDocument *doc, QDomElement *parent ) const {
  
  QDomElement o = doc->createElement("gobject");
  
  if (!type.isEmpty()) o.setAttribute( "type", type );
  if (!name.isEmpty()) o.setAttribute( "name", name );
  parent->appendChild(o);

  // create vertices
  for (int i=0; i<vertices.size(); ++i) 
    vertices[i].toXML( doc, &o );

  // create tags
  QHash<QString, QString>::const_iterator i = tags.constBegin();
  while (i != tags.constEnd()) {
     QDomElement tag = doc->createElement("tag");
     tag.setAttribute( "name", i.key() );
     tag.setAttribute( "value", i.value() );
     o.appendChild(tag);
     ++i;
  }

  // create children
  for (int i=0; i<children.size(); ++i) 
    children.at(i)->toXML( doc, &o );

  return 0;
}

QString DGObject::toCSVString() const {
  QString entry = type;
  entry += ",";
  entry += name;
  entry += ",";

  if (hasVertices()) {
    for (int i=0; i<vertices.size(); ++i) 
      entry += vertices[i].toCSVString();
  }
  else entry += "\n";
  return entry;  
}

inline DGObject& DGObject::operator+=(const DVertex5D<float>& o) { 
  for (int i=0; i<vertices.size(); ++i) vertices[i] += o;
  for (int i=0; i<children.size(); ++i) *(children[i]) += o;
  return *this; 
}

inline DGObject& DGObject::operator-=(const DVertex5D<float>& o) { 
  for (int i=0; i<vertices.size(); ++i) vertices[i] -= o;
  for (int i=0; i<children.size(); ++i) *(children[i]) -= o;
  return *this; 
}

inline DGObject& DGObject::operator*=(const DVertex5D<float>& o) { 
  for (int i=0; i<vertices.size(); ++i) vertices[i] *= o;
  for (int i=0; i<children.size(); ++i) *(children[i]) *= o;
  return *this; 
}

inline DGObject& DGObject::operator/=(const DVertex5D<float>& o) { 
  for (int i=0; i<vertices.size(); ++i) vertices[i] /= o;
  for (int i=0; i<children.size(); ++i) *(children[i]) /= o;
  return *this; 
}


//------------------------------------------------------------------------------------
// DGObjects
//------------------------------------------------------------------------------------

QString extractGObjectsFromBIX( const QString &fileName ) {
  QFile f(fileName);
  if ( !f.open( (QIODevice::OpenMode) QIODevice::ReadOnly) ) return "";

  QDomDocument dom;
  if ( !dom.setContent(&f) ) return "";

  QString gobjects_xml;

  QDomElement root = dom.documentElement();
  QDomElement node = root.firstChildElement("item");
  while ( !node.isNull() ) {
    if ( node.hasAttribute("type") && node.attribute("type") == "graphics" ) {
      gobjects_xml = node.firstChildElement("value").text();
      break;
    }
    node = node.nextSiblingElement("item");
  } // while

  return gobjects_xml;
}

int DGObjects::parse( QDomDocument *dom ) {
  
  clear();
  QDomElement root = dom->documentElement();
  QDomElement gobject_node = root.firstChildElement("gobject");
  while ( !gobject_node.isNull() ) {
    *this << QSharedPointer<DGObject>( new DGObject(gobject_node) );
    gobject_node = gobject_node.nextSiblingElement("gobject");
  } // while

  // now load all default render procs
  if (render_procs.size() > 0) {
    QHash<QString, DGObjectRenderProc>::const_iterator it = render_procs.begin();
    while ( it != render_procs.end() ) {
      setRenderProc( it.key(), it.value() );
      ++it;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------------

int DGObjects::fromFile( const QString &fileName ) {
  
  if ( !fileName.endsWith(".bix", Qt::CaseInsensitive) ) {
    // the file is simple gobjects XML
    QFile file(fileName);
    if ( !file.open(QIODevice::ReadOnly) ) return 1;
    QDomDocument dom;
    if ( !dom.setContent(&file) ) return 1;
    return parse( &dom );
  } else {
    // parse the BIX file, extract gobjects part and parse
    QString gobjects_xml = extractGObjectsFromBIX( fileName );
    return fromString( gobjects_xml );
  }   
}

int DGObjects::fromString( const QString &xml ) {
  QDomDocument dom;
  if ( !dom.setContent(xml) ) return 1;
  return parse( &dom );
}

QString DGObjects::toString() const {
  QDomDocument doc("BisqueML");
  QDomElement root = doc.createElement("response");
  doc.appendChild(root);

  for (int i=0; i<size(); ++i)
    this->at(i)->toXML( &doc, &root );
  return doc.toString();
}

int DGObjects::toFile( const QString &fileName ) const {
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return 1;
  QTextStream out(&file);
  out << toString();
  return 0;
}

//------------------------------------------------------------------------------------

void DGObjects::render( DGObjectRenderingOptions *opt ) const {
  for (int i=0; i<size(); ++i) {
    this->at(i)->render( opt );
  }
}

void DGObjects::setRenderProc( const QString &_type, DGObjectRenderProc new_render_proc ) {
  for (int i=0; i<size(); ++i) {
    this->at(i)->setRenderProc( _type, new_render_proc );
  }
}

template< typename F, typename P >
void DGObjects::walk_gobjects( QList< QSharedPointer<DGObject> > *l, F func, P params ) {
  for (int i=0; i<l->size(); ++i) {
    func( l->at(i), params );
    walk_gobjects( &l->at(i)->children, func, params );
  }
}

template< typename F, typename P >
void DGObjects::walker( F func, P params ) {
  walk_gobjects( this, func, params );
}

void DGObjects::paint(QPainter *p) const {

}

void DGObjects::paint(TDimImage *img) const {

}

//------------------------------------------------------------------------------------

inline DGObjects& DGObjects::operator+=(const DVertex5D<float>& o) { 
  for (int i=0; i<size(); ++i) *this->at(i) += o;
  return *this; 
}

inline DGObjects& DGObjects::operator-=(const DVertex5D<float>& o) { 
  for (int i=0; i<size(); ++i) *this->at(i) -= o;
  return *this; 
}

inline DGObjects& DGObjects::operator*=(const DVertex5D<float>& o) { 
  for (int i=0; i<size(); ++i) *this->at(i) *= o;
  return *this; 
}

inline DGObjects& DGObjects::operator/=(const DVertex5D<float>& o) { 
  for (int i=0; i<size(); ++i) *this->at(i) /= o;
  return *this; 
}

//------------------------------------------------------------------------------------

void add_count( QSharedPointer<DGObject>, unsigned int *c ) { (*c)++; }

unsigned int DGObjects::total_count() {
  unsigned int tc=0;
  walker( add_count, &tc );
  return tc;
}

//------------------------------------------------------------------------------------

void convert_to_csv( QSharedPointer<DGObject> o, QString *s ) { s->append( o->toCSVString() ); }

QString DGObjects::toCSVString() {
  QString s;
  walker( convert_to_csv, &s );
  return s;
}

//------------------------------------------------------------------------------------

void add_type( QSharedPointer<DGObject> o, QSet<QString> *s ) { s->insert( o->type ); }

QSet<QString> DGObjects::unique_type_names() {
  QSet<QString> s;
  walker( add_type, &s );
  return s;
}

void add_tag( QSharedPointer<DGObject> o, QSet<QString> *s ) { (*s) += QSet<QString>::fromList(o->tags.uniqueKeys()); }

QSet<QString> DGObjects::unique_tag_names() {
  QSet<QString> s;
  walker( add_tag, &s );
  return s;
}


//---------------------------------------------------------------------------------
// rendering options
//---------------------------------------------------------------------------------

DGObjectRenderingOptions::DGObjectRenderingOptions(): QObject(0) {
  size_multiplier = 1.0;
  transparency = 255;
  use_depth = false;

  color_override = false;
  color = QColor(255,0,0);

  probability_hide_below = 0;
  probability_use_transparency = false;
  probability_color_code = false;
  probability_color_high = QColor(255,0,0);
  probability_color_low = QColor(0,0,0);
  probability_tag_name = "probability";
}

inline unsigned char interpolate_color( double p, double h, double l ) {
  return (double)p*((h-l)/100.0) + l;
}

QColor probabilityColor( double prob, const QColor &h, const QColor &l ) {
  return QColor(
    interpolate_color( prob, h.red(), l.red() ),
    interpolate_color( prob, h.green(), l.green() ),
    interpolate_color( prob, h.blue(), l.blue() ) );
}

QColor DGObjectRenderingOptions::getGObjectColor(const DGObject &o) const {
  
  QColor c( o.tags.value("color", "#FF0000") );
  if (color_override) 
    c = color;
  
  double p  = o.tags.value(probability_tag_name, "100.0").toDouble();
  
  if (probability_color_code && o.tags.contains(probability_tag_name) ) 
    c = probabilityColor( p, probability_color_high, probability_color_low );

  c.setAlpha(transparency);

  if (probability_use_transparency)
    c.setAlpha(p*2.55);

  if (probability_hide_below>0 && p<probability_hide_below) 
    c.setAlpha(0);

  return c;
}

double DGObjectRenderingOptions::getGObjectLineWidth(const DGObject &o) const {
  return o.tags.value("line_width", "3.0").toFloat();
}

bool DGObjectRenderingOptions::isGObjectVisible(const DGObject *o) const {
  bool v = o->isVisible();
  if (exclude_types.size()>0 && exclude_types.contains(o->getType()) ) return false;
  if (include_types.size()>0 && !include_types.contains(o->getType()) ) return false;
  // test for color transparency
  return v;
}

