/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg     : 2-D array
  MG     : 2-D array
  m, v   : pointer to matrix m:2x2, v:2x1

OUTPUT:
  fit    : pointer to double (fit between g and G)
  mv, mV : pointer to double (mean)
  sv, sV : pointer to double (Standard Deviation)
  overlap: pointer to double


 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/27/2001 11:57:01 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 2
*****************************************************************************/

#ifndef REG_EVALUATE_FIT_AFFINE
#define REG_EVALUATE_FIT_AFFINE

/*
% Evaluate the fit between g and G for
% parameters m and v; m is a 2x2 matrix
% and v is a translation vector.
% Notation lower case letters refer to the first image g
% hence g(i,j) etc.
% Upper case letters refer to the second image G
% hence G(I,J) etc.
*/

//#define eps 2.2204e-016

template <typename T>
void laplacian_weight(reg::Matrix<T> *mg, reg::Matrix<T> *md) {
 
  int rows = mg->rows;
  int cols = mg->cols;  
  md->init(mg->rows, mg->cols);   

  T **g = (T **) mg->data;
  T **d = (T **) md->data;

  double mean = m_mean(g, rows, cols);
  double std  = m_std(g, rows, cols, mean);

  //d=g-mean(g(:));
  for (int i=1; i<rows; i++)
  for (int j=1; j<cols; j++)
    d[i][j] = (T)( (g[i][j] - mean) / std);

  //d=abs(d-smooth(d,1))+0.01;
  reg::Matrix<T> msg(mg);
  smooth(&msg, 1);

  T **sg = (T **) msg.data;
  for (int i=1; i<rows; i++)
  for (int j=1; j<cols; j++)
    d[i][j] = (T) (fabs(d[i][j]-sg[i][j])+0.01);
}

template <typename Timg, typename Tw>
void evalfitAFFINE(reg::Matrix<Timg> *mg, reg::Matrix<Timg> *MG, Tw **m, Tw **v,
             double *fit, double *mv, double *sv, double *mV, double *sV, double *overlap)


{
  Tw **g = (Tw **) mg->data;
  int g_rows = mg->rows;
  int g_cols = mg->cols;
  Tw **G = (Tw **) MG->data;
  int G_rows = MG->rows;
  int G_cols = MG->cols;

  // Set up the Laplacian error weight matrix
  reg::Matrix<Timg> md;
  laplacian_weight(mg, &md); 
  Tw **d = (Tw**) md.data;
  
  reg::Matrix<Timg> mD;
  laplacian_weight(MG, &mD); 
  Tw **D = (Tw**) mD.data;

  // Rotate (i,j) by m           
  // and translate by v=(dx,dy) to get (I,J)
  // we assume that g(i,j)=G(I,J)
  int numnum=0;
  int numtest=0;

  int temp[5];
  temp[1] = g_rows; temp[2] = g_cols; temp[3] = G_rows; temp[4] = G_cols;
  int mindim = a_min(temp, 1, 4);

  //define skip
  int skip=1;
  if (mindim>512) skip = 16;
  else if (mindim > 256) skip = 8;
  else if (mindim > 128) skip = 4;
  else if (mindim > 64) skip =2;

  int numok = 0;

  int ibot = round<int>(g_rows*0.25);
  int itop = round<int>(g_rows*0.75);
  int jbot = round<int>(g_cols*0.25);
  int jtop = round<int>(g_cols*0.75);

  int vvv_rows = (int)( ceil( ((double)(itop-ibot))/((double) skip) +1 )*
             ceil( ((double)(jtop-jbot))/((double) skip) +1 ) );
  int VVV_rows = vvv_rows ;
  std::vector<Tw> vvv(vvv_rows+1);
  std::vector<Tw> VVV(vvv_rows+1);
  std::vector<Tw> DDD(vvv_rows+1);

  for(int i=ibot;i<=itop;i+=skip)
  for(int j=jbot;j<=jtop;j+=skip) {
    numtest=numtest+1;

    // but we need to round and interpolate cause
    // w is most likely not an integer vector
    //w=m*([i,j]')+v;
    double ww[3];
    ww[1] = m[1][1]*((double)i) + m[1][2]*((double)j) + v[1][1];
    ww[2] = m[2][1]*((double)i) + m[2][2]*((double)j) + v[2][1];

    //ww=w;
    //w=floor(w);
    double w[3];
    w[1] = floor(ww[1]);
    w[2] = floor(ww[2]);

    int I = round<int>(w[1]);
    int J = round<int>(w[2]);

    if (I>1 && I<G_rows && J>1 && J<G_cols) {
      // the point (i,j) maps into a point (I,J)
      // that is in the range of G:  1<I<rG  and 1<J<cG
      // this is an acceptable point so compute the
      // difference g(i,j)-G(I,J); if the parameters m and v
      // were perfect we would have a difference = 0

      // Keep track of the number of acceptable points
      numok++;

      // get the interpolation weights for (I,J)
      // (I+1,J) (I,J+1) and (I+1,J+1)
      double ax = ww[1] - w[1];
      double ay = ww[2] - w[2];
      double GG =  (1.0-ax) * (1.0-ay) * ((double) G[I][J]);
      GG += (ax) * (1.0-ay) * ((double) G[I+1][J]);
      GG += (1.0-ax) * (ay) * ((double) G[I][J+1]);
      GG += (ax) * (ay) * ((double) G[I+1][J+1]);
      numnum=numnum+1;
      vvv[numnum]=g[i][j];
      VVV[numnum]=(Tw) GG;
      DDD[numnum]=D[I][J]+d[i][j];
    }
  } 

  if (numok>0) {
    // difference of normalized intensities
    *mv = a_mean(&vvv[0], 1, numnum);
    *sv = a_std (&vvv[0], 1, numnum, *mv);
    *mV = a_mean(&VVV[0], 1, numnum);
    *sV = a_std (&VVV[0], 1, numnum, *mV);

    for (int i=1; i<=numnum; i++) {
       vvv[i]=(Tw) ( (vvv[i]- *mv)/(*sv + REG_EPS) );
       VVV[i]=(Tw) ( (VVV[i]- *mV)/(*sV + REG_EPS) );
    }
    a_diff(&vvv[0], &VVV[0], &vvv[0], 1, numnum);
    a_abs (&vvv[0], &vvv[0], 1, numnum);
    for (int i=1; i<=numnum; i++) vvv[i] = vvv[i] / DDD[i];

    *fit = 2*a_mean(&vvv[0], 1, numnum);
  } else {
   // yikes the images don't overlap after scaling rotating and
   // translating - this is bad so set fit equal to a very big value
   *fit = 1.0e08;
  }

  *overlap = ((double) numok)/((double) numtest);
}
      
#endif // REG_EVALUATE_FIT_AFFINE
