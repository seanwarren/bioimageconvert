/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg - 2-D array

OUTPUT:
  points

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   08/10/2001 15:06:32 - use of TPointA
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_FIND_MAXIMA
#define REG_FIND_MAXIMA

#include <cmath>

template< typename Timg, typename Tpoint >
int findmaxima(const reg::Matrix<Timg> *mg, int numpoints, std::deque< reg::Point<Tpoint> > *points ) {
  Timg **g = (Timg **) mg->data;
  int rg = mg->rows;
  int cg = mg->cols;

  double num = ceil(sqrt((double)numpoints)+2.0);
  int lenx = (int) floor(rg/num);
  int leny = (int) floor(cg/num);

  Timg **gg = m_init<Timg>(lenx+1, leny+1);

  for (int i=2; i<=num-1; i++)
  for (int j=2; j<=num-1; j++) {
    int xc = (i-1)*lenx;
    int yc = (j-1)*leny;

    #pragma omp parallel for default(shared)
    for (int ii=xc; ii<=xc+lenx; ii++)
      for (int jj=yc; jj<=yc+leny; jj++)
        gg[ii-xc+1][jj-yc+1] = g[ii][jj];

    reg::Point<Tpoint> p = m_max_point<Timg, reg::Point<Tpoint> >(gg, lenx, leny);
    points->push_back( reg::Point<Tpoint>( p.y+yc-1, p.x+xc-1) );
  }
  m_free(gg);
  return 0;
}

#endif // REG_FIND_MAXIMA
