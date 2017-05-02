/*****************************************************************************
  code by Dima V. Fedorov, 2002

INPUT: 
  xyA, XYA - pointer to TPointA

OUTPUT:
  r, dx, dy - pointers to double

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   08/10/2001 15:06:32 - use of TPointA
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_GET_PARAMS_ST_H
#define REG_GET_PARAMS_ST_H

template< typename Tw, typename Tpoint >
void getSTparameters( const std::deque< reg::Point<Tpoint> > *points1, //TPointA *xyA, 
                      const std::deque< reg::Point<Tpoint> > *points2, //TPointA *XYA, 
                      double *r, double *dx, double *dy ) {
  // This routine uses a least squares minimization to
  // estimate the parameters r,dx,dy that describe
  // the matching between (x,y) and (X,Y) in the form
  // X(i)  =  [ r*1 r*0 ] [ x(i) ] + [ dx ]
  // Y(i)  =  [-r*0 r*1 ] [ y(i) ] + [ dy ]
  //
  //          r = scaling factor
  //         dx = x translation
  //         dy = y translation

  int lenx = points1->size();
  
  Tw **mat  = m_init<Tw>(2*lenx, 4);
  Tw **b    = m_init<Tw>(2*lenx, 1);
  Tw **v    = m_init<Tw>(4, 1);
  Tw **temp = m_init<Tw>(4, 2*lenx);

  for (int k=1; k<=lenx; k++) {
    mat[2*k-1][1] = (*points1)[k-1].y; //xy[k].x;
    mat[2*k-1][2] = 0;
    mat[2*k-1][3] = 1;
    mat[2*k-1][4] = 0;
    b[2*k-1][1] = (*points2)[k-1].y; //XY[k].x;

    mat[2*k][1] = (*points1)[k-1].x; //xy[k].y;
    mat[2*k][2] = 0;
    mat[2*k][3] = 0;
    mat[2*k][4] = 1;   
    b[2*k][1] = (*points2)[k-1].x; //XY[k].y;
  }

  //v=pinv(mat)*b;
  pinv(mat, 2*lenx, 4, temp);
  m_mult(temp, b, 4, 2*lenx, 2*lenx, 1, v);

  *r  = sqrt(v[1][1] * v[1][1] + v[2][1] * v[2][1]);
  *dx = v[3][1];
  *dy = v[4][1];

  m_free(temp);
  m_free(mat);
  m_free(b);
  m_free(v);
}

#endif // REG_GET_PARAMS_ST_H


