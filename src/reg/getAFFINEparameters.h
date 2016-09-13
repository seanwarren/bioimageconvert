/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Original C code by Leila M.G. Fonseca
  C coding by Dima V. Fedorov, 2001

INPUT: 
  xyA, XYA - pointer to TPointA

OUTPUT:
  m, vo - pointers to matrix m-2x2, vo-2x1

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/27/2001 10:45:30 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatment
   04/23/2002 16:37:36 - new version for treatment of strange bug

 Ver : 3
*****************************************************************************/

#ifndef REG_GET_PARAMS_AF_H
#define REG_GET_PARAMS_AF_H

#include "recipes.h"

template< typename Tw, typename Tpoint >
void getAFFINEparameters2( const std::deque< reg::Point<Tpoint> > *points1, 
                           const std::deque< reg::Point<Tpoint> > *points2,
                           Tw **m, Tw **vo ) {
// This routine uses a least squares minimization to
// estimate the parameters r,c,s,dx,dy that describe
// the matching between (x,y) and (X,Y) in the form
// X(i)  =  [ m11 m12 ] [ x(i) ]  +[ m13]
// Y(i)  =  [ m21 m22 ] [ y(i) ]  +[ m23]

  int err;
  int lenx = points1->size();
  
  Tw **mat  = m_init<Tw>(2*lenx, 6);
  Tw **b    = m_init<Tw>(2*lenx, 1);
  Tw **v    = m_init<Tw>(6, 1);

  for (int k=1; k<=lenx; k++) {
    mat[2*k-1][1] = (*points1)[k-1].y; //xy[k].x;
    mat[2*k-1][2] = (*points1)[k-1].x; //xy[k].y;
    mat[2*k-1][3] = 1;
    mat[2*k-1][4] = 0;
    mat[2*k-1][5] = 0;
    mat[2*k-1][6] = 0;

    mat[2*k][1] = 0;
    mat[2*k][2] = 0;
    mat[2*k][3] = 0;
    mat[2*k][4] = (*points1)[k-1].y; //xy[k].x;
    mat[2*k][5] = (*points1)[k-1].x; //xy[k].y;
    mat[2*k][6] = 1;   

    b[2*k-1][1] = (*points2)[k-1].y; //XY[k].x;
    b[2*k][1] = (*points2)[k-1].x; //XY[k].y;
  }

  //v=pinv(mat)*b;
  Tw **temp = m_init<Tw>(6, 2*lenx);
  err = pinv(mat, 2*lenx, 6, temp);
  m_mult(temp, b, 6, 2*lenx, 2*lenx, 1, v);
  m_free(temp);

  m[1][1] = v[1][1];
  m[1][2] = v[2][1];
  m[2][1] = v[4][1];
  m[2][2] = v[5][1];

  vo[1][1] = v[3][1];
  vo[2][1] = v[6][1];

  m_free(mat);
  m_free(b);
  m_free(v);
}

template< typename Tw, typename Tpoint >
void getAFFINEparameters( const std::deque< reg::Point<Tpoint> > *points1, //TPointA *xyA, 
                           const std::deque< reg::Point<Tpoint> > *points2, //TPointA *XYA, 
                           Tw **m, Tw **vo ) {
  int k = (int) points1->size();
  int	kk = 2 * k;

  Tw **b = m_init<Tw>(kk, 6);
  std::vector<Tw> a(kk+1);
  
  // Load the CP in the vectors	 
  for(int i=0; i<k; i++) {
    a[i*2] = (*points2)[i].y; //XY[i+1].x;
    a[i*2+1] = (*points2)[i].x; //XY[i+1].y;

    b[i*2][0] = (*points1)[i].y; //xy[i+1].x;
    b[i*2][1] = (*points1)[i].x; //xy[i+1].y;
    b[i*2][2]=1.;
    b[i*2][3]=0.;
    b[i*2][4]=0.;
    b[i*2][5]=0.;

    b[i*2+1][0]=0.;
    b[i*2+1][1]=0.;
    b[i*2+1][2]=0.;
    b[i*2+1][3] = (*points1)[i].y; //xy[i+1].x;
    b[i*2+1][4] = (*points1)[i].x; //xy[i+1].y;
    b[i*2+1][5]=1.;
  }
  
  Tw **btb = m_init<Tw>(6, 6); // bTb ( b transposto b )

  //  Calculate bTb
  Tw aux;
  for(int i=0;i<6;i++) 
  for(int j=0;j<6;j++) {
    aux = 0;
    for(int l=0;l<kk;l++) aux  += b[l][i]*b[l][j];
	  btb[i+1][j+1] = aux;
  }

  // Calculate bTa
  Tw bta[6];
  for(int i=0;i<6;i++) {
    aux=0;
    for(int l=0;l<kk;l++) aux += a[l]*b[l][i];	// bTa
    bta[i] = aux;
  }

  Tw **y = m_init<Tw>(6, 6); // inverse matrix
  std::vector<int> indx(7);
  std::vector<Tw> col(7);

  // LU decomposition of btb 
  Tw d;
  ludcmp(btb,6,&indx[0],&d);

  // find the inverse of a matrix column by column
  for(int j=1;j<=6;j++) {
    for(int i=1;i<=6;i++) col[i]=0.0;
    col[j]=1.0;
    lubksb(btb,6,&indx[0],&col[0]);
    for(int i=1;i<=6;i++) y[i][j]=col[i];
  }

  // calculate y*bta
  Tw result[6];
  for (int i=0;i<6;i++) {
    aux=0;
    for(int j=0;j<6;j++)
      aux += y[i+1][j+1]*bta[j];
    result[i] = aux;
  }

  m[1][1] = result[0];
  m[1][2] = result[1];
  m[2][1] = result[3];
  m[2][2] = result[4];

  vo[1][1] = result[2];
  vo[2][1] = result[5];

  m_free(btb);
  m_free(y);
  m_free(b);
}

#endif // REG_GET_PARAMS_AF_H
