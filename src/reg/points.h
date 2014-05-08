/*****************************************************************************
 Point and point list classes

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   10/20/2001 13:05:33 - First creation

 Ver : 1
*****************************************************************************/

#ifndef REG_POINTS_H
#define REG_POINTS_H

#include <cstdlib>

#include <string>
#include <list>
#include <deque>
#include <sstream>
#include <iostream>
#include <fstream>

#include <BioImageCore>

using namespace bim;

namespace reg {

//****************************************************************************
// Point
//****************************************************************************

template <typename T>
class Point {
public:
  typedef T data_type;

public:
  Point(): x(0), y(0) {}
  Point( const T &_x, const T &_y ): x(_x), y(_y) {}

  bool operator==(const Point<T> &p) const { return ( p.x==this->x && p.y==this->y); }
  bool operator!=(const Point<T> &p) const { return !(*this == p); }
  void operator-=(const Point<T> &p) { this->x-=p.x; this->y-=p.y; }
  void operator-=(const T &d) { this->x-=d; this->y-=d; }
  void operator+=(const Point<T> &p) { this->x+=p.x; this->y+=p.y; }
  void operator+=(const T &d) { this->x+=d; this->y+=d; }

public:
  T x;
  T y;
};

//****************************************************************************
// PointList
//****************************************************************************

template <typename T>
class PointList : public std::deque< Point<T> > {
public:
  typedef T point_type;
  typedef typename std::deque< Point<T> >::iterator iterator;
  typedef typename std::deque< Point<T> >::const_iterator const_iterator;
  typedef typename std::deque< Point<T> >::size_type size_type;
  
public:
  // constructors
  explicit PointList(): std::deque< Point<T> >() {}
  PointList( size_type _count): std::deque< Point<T> >(_count) {}
  PointList( size_type _count, const T& _val): std::deque< Point<T> >(_count, _val) {}
  PointList( const std::deque< Point<T> >& _Right ): std::deque< Point<T> >(_Right) {}

  template<class InputIterator>
  PointList( InputIterator _First, InputIterator _Last ): std::deque< Point<T> >(_First, _Last) {}

  void operator-=(const Point<T> &p);
  void operator-=(const T &d);
  void operator+=(const Point<T> &p);
  void operator+=(const T &d);

  // I/O
  std::string join( const std::string &line_sep="\n" ) const;
  std::string toString() const { return join(); }
  bool toFile( const std::string &file_name ) const;
  //bool fromFile( const std::string &file_name );
};

template <typename T>
void PointList<T>::operator-=(const Point<T> &p) {
  iterator it = this->begin();
  while (it != this->end()) {
    (*it) -= p;
    ++it;
  }
}

template <typename T>
void PointList<T>::operator-=(const T &d) {
  iterator it = this->begin();
  while (it != this->end()) {
    (*it) -= d;
    ++it;
  }
}

template <typename T>
void PointList<T>::operator+=(const Point<T> &p) {
  iterator it = this->begin();
  while (it != this->end()) {
    (*it) += p;
    ++it;
  }
}

template <typename T>
void PointList<T>::operator+=(const T &d) {
  iterator it = this->begin();
  while (it != this->end()) {
    (*it) += d;
    ++it;
  }
}

template <typename T>
std::string PointList<T>::join( const std::string &line_sep ) const {
  std::string s;
  const_iterator it = this->begin();
  int index=0;
  while (it != this->end()) {
    //s += xstring::xprintf("(%f, %f)", it->x, it->y);
    s += xstring::xprintf("<gobject type='point' name='%d'><vertex x='%f' y='%f'/></gobject>", index, it->x, it->y );
    s += line_sep;
    ++it;
    ++index;
  }
  return s;
}

template <typename T>
bool PointList<T>::toFile( const std::string &file_name ) const {
  std::ofstream f;
  f.open(file_name.c_str());
  if (!f.is_open()) return false;
  f << this->join("\n");
  f.close();
  return true;
}

} // namespace reg

#endif // REG_POINTS_H


