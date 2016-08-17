/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg   - 2-D array

OUTPUT:
  mres - 2-D array

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatmen

 Ver : 2
*****************************************************************************/

#ifndef REG_RESOLVE_NT
#define REG_RESOLVE_NT

#include <cmath>

template <typename T>
int resolvent(reg::Matrix<T> *mg, reg::Matrix<T> *mres) {
  // get the least-squares operator norm surface
  int dist=1;

  T** g = (T **) mg->data;
  T** res = (T **) mres->data;
  if ( (g == NULL) || (res == NULL) ) return 1;

  int rg = mg->rows;
  int cg = mg->cols;

  smooth(mg, 1);

  // step one get gx,gy
  T **gx = m_init<T>(rg, cg);
  m_zeros(gx, rg, cg);

  T **gy = m_init<T>(rg, cg);
  m_zeros(gy, rg, cg);

  T **gxx = m_init<T>(rg, cg);
  m_zeros(gxx, rg, cg);

  T **gxy = m_init<T>(rg, cg);
  m_zeros(gxy, rg, cg);

  T **gyy = m_init<T>(rg, cg);
  m_zeros(gyy, rg, cg);

  #pragma omp parallel for default(shared)
  for(int i=2; i<=rg-1; i++)
  for(int j=2; j<=cg-1; j++) {
    gx[i][j] = (T)((g[i+1][j] - g[i-1][j]) / 2.0);
    gy[i][j] = (T)((g[i][j+1] - g[i][j-1]) / 2.0);
  }

  T **p = m_init<T>(rg, cg);

  for (int i=-dist; i<=dist; i++)
  for (int j=-dist; j<=dist; j++) {
    m_ones(p, rg, cg);

    #pragma omp parallel for default(shared)
    for(int im=dist+1; im<=rg-dist; im++)
    for(int jm=dist+1; jm<=cg-dist; jm++) {
      p[im][jm] = gx[im][jm]*gx[im+i][jm+j] + gy[im][jm]*gy[im+i][jm+j];
    }
       
    m_max_scalar(p, p, rg, cg, 0);

    #pragma omp parallel for default(shared)
    for(int im=dist+1; im<=rg-dist; im++)
    for(int jm=dist+1; jm<=cg-dist; jm++) {
      gxx[im][jm] = gxx[im][jm]+p[im][jm] * gx[im+i][jm+j]*gx[im+i][jm+j];
      gxy[im][jm] = gxy[im][jm]+p[im][jm] * gx[im+i][jm+j]*gy[im+i][jm+j];
      gyy[im][jm] = gyy[im][jm]+p[im][jm] * gy[im+i][jm+j]*gy[im+i][jm+j];
    }
  }

  #pragma omp parallel for default(shared)
  for(int i=1; i<=rg; i++)
  for(int j=1; j<=cg; j++)
    res[i][j] = (T)( fabs(gxx[i][j]*gyy[i][j]-gxy[i][j]*gxy[i][j]) /
      (REG_EPS + sqrt(gxx[i][j]*gxx[i][j]+gyy[i][j]*gyy[i][j]+2.0*gxy[i][j]*gxy[i][j]) ));

  #pragma omp parallel for default(shared)
  for(int i=1; i<=rg; i++)
  for(int j=1; j<=cg; j++)
    res[i][j] = (T)( res[i][j] / sqrt(REG_EPS+gxx[i][j]+gyy[i][j] ));

  m_free(gx);
  m_free(gy);
  m_free(gxx);
  m_free(gxy);
  m_free(gyy);
  m_free(p);

  return 0; 
}

#endif // REG_RESOLVE_NT
