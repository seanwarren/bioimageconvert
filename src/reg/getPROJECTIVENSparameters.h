/*****************************************************************************
  Estimate 8 parameter projective transformation using least squares method
  NS - 8 parameter, No Scale
    
  Copyright by Dima V. Fedorov, 2003

INPUT: 
  xyA, XYA - pointer to TPointA

OUTPUT:
  m, vo - pointers to matrix m-3x3, vo-3x1

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   16/10/2003 14:11:00 - First creation

 Ver : 1
*****************************************************************************/

#ifndef REG_GET_PARAMS_PJ_H
#define REG_GET_PARAMS_PJ_H

template< typename Tw, typename Tpoint >
void getPROJECTIVENSparameters2( const std::deque< reg::Point<Tpoint> > *points1, //TPointA *xyA, 
                                 const std::deque< reg::Point<Tpoint> > *points2, //TPointA *XYA, 
                                 Tw **m, Tw **vo ) {

// This routine uses a least squares minimization to
// estimate the parameters that describe
// the matching between (x,y) and (X,Y) in the form:
//   X(i)  =  [ m11 m12  m13] [ x(i) ]
//   Y(i)  =  [ m21 m22  m23] [ y(i) ]
//   1     =  [ m31 m32  1  ] [ 1    ]
// Using Jacobian:
//   [ x y 1 0 0 0 -x^2 -xy ]
//   [ 0 0 0 x y 1 -xy -y^2 ]

  int lenx = points1->size();

  Tw **mat  = m_init<Tw>(2*lenx, 8);
  Tw **b    = m_init<Tw>(2*lenx, 1);
  Tw **vd   = m_init<Tw>(8, 1);
  for (int i=1; i<9; i++) vd[i][0] = 0.0;
  Tw **temp = m_init<Tw>(8, 2*lenx);

  for (int k=1; k<=lenx; k++) {
    mat[2*k-1][1] = (*points1)[k-1].y; //xy[k].x;
    mat[2*k-1][2] = (*points1)[k-1].x; //xy[k].y;
    mat[2*k-1][3] = 1;
    mat[2*k-1][4] = 0;
    mat[2*k-1][5] = 0;
    mat[2*k-1][6] = 0;
    mat[2*k-1][7] = -1 * ((*points1)[k-1].y * (*points1)[k-1].y); //-1 * (xy[k].x * xy[k].x);
    mat[2*k-1][8] = -1 * ((*points1)[k-1].y * (*points1)[k-1].x); //-1 * (xy[k].x * xy[k].y);

    mat[2*k][1] = 0;
    mat[2*k][2] = 0;
    mat[2*k][3] = 0;
    mat[2*k][4] = (*points1)[k-1].y; //xy[k].x;
    mat[2*k][5] = (*points1)[k-1].x; //xy[k].y;
    mat[2*k][6] = 1;
    mat[2*k][7] = -1 * ((*points1)[k-1].y * (*points1)[k-1].x); // -1 * (xy[k].x * xy[k].y);
    mat[2*k][8] = -1 * ((*points1)[k-1].x * (*points1)[k-1].x); // -1 * (xy[k].y * xy[k].y);    

    b[2*k-1][1] = (*points2)[k-1].y; //XY[k].x;
    b[2*k][1] = (*points2)[k-1].x; //XY[k].y;
  }

  //v=pinv(mat)*b;
  int err = pinv(mat, 2*lenx, 8, temp);
  m_mult(temp, b, 8, 2*lenx, 2*lenx, 1, vd);

  m[1][1] = vd[1][1];
  m[1][2] = vd[2][1];
  m[1][3] = vd[3][1];
  m[2][1] = vd[4][1];
  m[2][2] = vd[5][1];
  m[2][3] = vd[6][1];
  m[3][1] = vd[7][1];
  m[3][2] = vd[8][1];
  m[3][3] = 1;


  vo[1][1] = vd[3][1];
  vo[2][1] = vd[6][1];
  vo[3][1] = 0;

  m_free(temp);
  m_free(mat);
  m_free(b);
  m_free(vd);
}

template< typename Tw, typename Tpoint >
void getPROJECTIVENSparameters( const std::deque< reg::Point<Tpoint> > *points1, //TPointA *xyA, 
                                const std::deque< reg::Point<Tpoint> > *points2, //TPointA *XYA, 
                                Tw **m, Tw **vo ) {
  int k = points1->size()-1;
  int	kk = 2 * k;

  Tw **b = m_init<Tw>(kk, 8);
  std::vector<Tw> a(kk+1);

  // Load the CP in the vectors	 
  for(int i=0;i<k;i++) {
    a[i*2]   = (*points2)[i].y; //XY[i+1].x;
    a[i*2+1] = (*points2)[i].x; //XY[i+1].y;

    b[i*2][0] = (*points1)[i].y; //xy[i+1].x;
    b[i*2][1] = (*points1)[i].x; //xy[i+1].y;
    b[i*2][2]=1.;
    b[i*2][3]=0.;
    b[i*2][4]=0.;
    b[i*2][5]=0.;
    b[i*2][6] = -1 * ((*points1)[i].y * (*points1)[i].y); // -1 * (xy[i+1].x * xy[i+1].x);
    b[i*2][7] = -1 * ((*points1)[i].y * (*points1)[i].x); // -1 * (xy[i+1].x * xy[i+1].y);

    b[i*2+1][0]=0.;
    b[i*2+1][1]=0.;
    b[i*2+1][2]=0.;
    b[i*2+1][3] = (*points1)[i].y; //xy[i+1].x;
    b[i*2+1][4] = (*points1)[i].x; //xy[i+1].y;
    b[i*2+1][5]=1.;
    b[i*2+1][6] = -1 * ((*points1)[i].y * (*points1)[i].x); // -1 * (xy[i+1].x * xy[i+1].y);
    b[i*2+1][7] = -1 * ((*points1)[i].x * (*points1)[i].x); // -1 * (xy[i+1].y * xy[i+1].y);
  }
  
  Tw **btb = m_init<Tw>(8, 8); // bTb ( b transposto b )

  //  Calculate bTb
  for(int i=0;i<8;i++) 
  for(int j=0;j<8;j++){
    Tw aux = 0.;
    for(int l=0;l<kk;l++) aux  += b[l][i]*b[l][j];
	  btb[i+1][j+1] = aux;
  }

  // Calculate bTa
  Tw bta[8];
  for(int i=0;i<8;i++) {
    Tw aux=0;
    for(int l=0;l<kk;l++) aux += a[l]*b[l][i];	// bTa
    bta[i] = aux;
  }

  Tw **y = m_init<Tw>(8, 8); // inverse matrix
  std::vector<int> indx(9);
  std::vector<Tw> col(9);

  // LU decomposition of btb 
  Tw d;
  ludcmp(btb, 8, &indx[0], &d);

  // find the inverse of a matrix column by column
  for(int j=1;j<=8;j++) {
    for(int i=1;i<=8;i++) col[i]=0.0;
    col[j]=1.0;
    lubksb(btb,8,indx,col);
    for(int i=1;i<=8;i++) y[i][j]=col[i];
  }

  // calculate y*bta
  Tw result[8];
  for (int i=0;i<8;i++) {
    Tw aux=0;
    for(int j=0;j<8;j++) {
      aux += y[i+1][j+1]*bta[j];
    }
    result[i] = aux;
  }

  m[1][1] = result[0];
  m[1][2] = result[1];
  m[1][3] = result[2];
  m[2][1] = result[3];
  m[2][2] = result[4];
  m[2][3] = result[5];
  m[3][1] = result[6];
  m[3][2] = result[7];
  m[3][3] = 1.0;


  vo[1][1] = m[1][3];
  vo[2][1] = m[2][3];
  vo[3][1] = 0.0;

  m_free(btb);
  m_free(y);
  m_free(b);
}

#endif // REG_GET_PARAMS_PJ_H

