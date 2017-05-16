/*****************************************************************************
 Primitives

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2010 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   2010-07-29 17:18:22 - First creation

 Ver : 1
*****************************************************************************/

#ifndef BIM_PRIMITIVES
#define BIM_PRIMITIVES

#include "xtypes.h"

#include <algorithm>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace bim {

template <typename T>
class Point {
public:
    Point(T _x = 0, T _y = 0, T _z = 0, T _t = 0, T _c = 0) : x(_x), y(_y), z(_z), t(_t), c(_c) { }

    bool operator== (const T &v) const { return this->x == v && this->y == v && this->z == v && this->t == v && this->c == v; }
    bool operator!= (const T &v) const { return this->x != v || this->y != v || this->z != v || this->t != v || this->c != v; }
    bool operator<  (const T &v) const { return this->x < v && this->y < v && this->z < v && this->t < v && this->c < v; }
    bool operator>  (const T &v) const { return this->x > v && this->y > v && this->z > v && this->t > v && this->c > v; }
    bool operator<= (const T &v) const { return this->x <= v && this->y <= v && this->z <= v && this->t <= v && this->c <= v; }
    bool operator>= (const T &v) const { return this->x >= v && this->y >= v && this->z >= v && this->t >= v && this->c >= v; }

    bool operator== (const Point<T> &p) const { return this->x == p.x && this->y == p.y && this->z == p.z && this->t == p.t && this->c == p.c; }
    bool operator!= (const Point<T> &p) const { return this->x != p.x || this->y != p.y || this->z != p.z || this->t != p.t || this->c != p.c; }
    bool operator<  (const Point<T> &p) const { return this->x < p.x && this->y < p.y && this->z < p.z && this->t < p.t && this->c < p.c; }
    bool operator>  (const Point<T> &p) const { return this->x > p.x && this->y > p.y && this->z > p.z && this->t > p.t && this->c > p.c; }
    bool operator<= (const Point<T> &p) const { return this->x <= p.x && this->y <= p.y && this->z <= p.z && this->t <= p.t && this->c <= p.c; }
    bool operator>= (const Point<T> &p) const { return this->x >= p.x && this->y >= p.y && this->z >= p.z && this->t >= p.t && this->c >= p.c; }

    Point<T> operator+ (const Point<T> &p) { this->x += p.x; this->y += p.y; this->z += p.z; this->t += p.t; this->c += p.c; }
    Point<T> operator- (const Point<T> &p) { this->x -= p.x; this->y -= p.y; this->z -= p.z; this->t -= p.t; this->c -= p.c; }
    Point<T> operator/ (const Point<T> &p) { this->x /= p.x; this->y /= p.y; this->z /= p.z; this->t /= p.t; this->c /= p.c; }
    Point<T> operator* (const Point<T> &p) { this->x *= p.x; this->y *= p.y; this->z *= p.z; this->t *= p.t; this->c *= p.c; }

public:
    T x, y, z, t, c;
};

template <typename T>
class Rectangle {
public:
    Rectangle(const Point<T> &_p1 = Point<T>(), const Point<T> &_p2 = Point<T>()) : p1(_p1), p2(_p2) {}

    bool operator== (const T &v) const { return this->p1 == v && this->p2 == v; }
    bool operator!= (const T &v) const { return this->p1 != v || this->p2 != v; }
    bool operator<  (const T &v) const { return this->p1 < v && this->p2 < v; }
    bool operator>  (const T &v) const { return this->p1 > v && this->p2 > v; }
    bool operator<= (const T &v) const { return this->p1 <= v && this->p2 <= v; }
    bool operator>= (const T &v) const { return this->p1 >= v && this->p2 >= v; }

    bool operator== (const Rectangle<T> &r) const { return this->p1 == r.p1 && this->p2 == r.p2; }
    bool operator!= (const Rectangle<T> &r) const { return this->p1 != r.p1 || this->p2 != r.p2; }
    bool operator<  (const Rectangle<T> &r) const { return this->p1 < r.p1 && this->p2 < r.p2; }
    bool operator>  (const Rectangle<T> &r) const { return this->p1 > r.p1 && this->p2 > r.p2; }
    bool operator<= (const Rectangle<T> &r) const { return this->p1 <= r.p1 && this->p2 <= r.p2; }
    bool operator>= (const Rectangle<T> &r) const { return this->p1 >= r.p1 && this->p2 >= r.p2; }

    Rectangle<T> operator+ (const Point<T> &p) { this->p1 += p; this->p2 += p; }
    Rectangle<T> operator- (const Point<T> &p) { this->p1 -= p; this->p2 -= p; }
    Rectangle<T> operator/ (const Point<T> &p) { this->p1 /= p; this->p2 /= p; }
    Rectangle<T> operator* (const Point<T> &p) { this->p1 *= p; this->p2 *= p; }

    Rectangle<T> operator+ (const Rectangle<T> &r) { this->p1 += r.p1; this->p2 += r.p2; }
    Rectangle<T> operator- (const Rectangle<T> &r) { this->p1 -= r.p1; this->p2 -= r.p2; }
    Rectangle<T> operator/ (const Rectangle<T> &r) { this->p1 /= r.p1; this->p2 /= r.p2; }
    Rectangle<T> operator* (const Rectangle<T> &r) { this->p1 *= r.p1; this->p2 *= r.p2; }

    T width()  const { return std::max(p2.x, p1.x) - std::min(p2.x, p1.x) + 1; }
    T height() const { return std::max(p2.y, p1.y) - std::min(p2.y, p1.y) + 1; }
    T depth()  const { return std::max(p2.z, p1.z) - std::min(p2.z, p1.z) + 1; }

public:
    Point<T> p1, p2;
};

} // namespace bim

#endif // BIM_PRIMITIVES
