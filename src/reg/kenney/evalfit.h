/*****************************************************************************
  Original Matlab code by Charles Kenney 
  First converted by Chao Huang, 2001
  Posterior modif. Dima V. Fedorov, 2001

INPUT:
  mg     : 2-D array
  MG     : 2-D array
  r,c,s  : parameters for scale-rotation matrix
  dx, dy : translation parameters

OUTPUT:
  fit    : pointer to double (fit between g and G)
  mv, mV : pointer to double (mean)
  sv, sV : pointer to double (Standard Deviation)


 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   08/15/2001 22:31:19 - Bug corrected, G[I][J] -> G[I][I]
   10/02/2001 19:12:00 - GCC warnings and Linux treatment
   10/08/2001 13:46:00 - Added seconf funct, only for resized evalfit array

 Ver : 4
*****************************************************************************/

#ifndef REG_EVALUATE_FIT_RST
#define REG_EVALUATE_FIT_RST

/*
 Evaluate the fit between g and G for
 parameters m and v; m is a scale-rotation matrix
 and v is a translation vector.
 Notation: lower case letters refer to the first image g
 hence g(i,j) etc.
 Upper case letters refer to the second image G
 hence G(I,J) etc.

 Rotate (i,j) by m            
 and translate by v=(dx,dy) to get (I,J)
 we assume that g(i,j)=G(I,J)
*/

//#define eps 2.2204e-016

template <typename Timg, typename Tw>
void evalfit(const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *MG, double r, double c, double s, double dx, double dy,
             double *fit, double *mv, double *sv, double *mV, double *sV, double *overlap) {
  int numnum = 0;
  
  Timg **g = (Timg **) mg->data;
  int g_rows = mg->rows;
  int g_cols = mg->cols;
  Timg **G = (Timg **) MG->data;
  int G_rows = MG->rows;
  int G_cols = MG->cols;

  //define translation
  Tw v[3][2];  
  v[1][1] = (Tw) dx;
  v[2][1] = (Tw) dy;

  //define rotation matrix
  Tw m[3][3];
  m[1][1] = (Tw)(r*c);
  m[1][2] = (Tw)(r*s);
  m[2][1] = (Tw)(-r*s);
  m[2][2] = (Tw)(r*c);

  int temp[4];
  temp[0] = g_rows; temp[1] = g_cols; temp[2] = G_rows; temp[3] = G_cols;
  int mindim = a_min(temp, 0, 3);

  //define skip
  int skip = std::max<int>(1, round<int>(mindim/32.0));

  int ibot = round<int>(g_rows*0.25);
  int itop = round<int>(g_rows*0.75);
  int jbot = round<int>(g_cols*0.25);
  int jtop = round<int>(g_cols*0.75);

  int vvv_rows = (int) (ceil( ((double)(itop-ibot))/((double) skip) +1.0 )*
             ceil( ((double)(jtop-jbot))/((double) skip) +1.0 ) );
  int VVV_rows = vvv_rows ;
  std::vector<Tw> vvv(vvv_rows+1);
  std::vector<Tw> VVV(VVV_rows+1);

  int numok = 0;
  int difference = 0;
  int numtest=0;
  for (int i=ibot;i<=itop;i+=skip)
  for (int j=jbot;j<=jtop;j+=skip) {
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
    }
  } 

  if (numok>0) {

    //rescale
    *mv = a_mean(&vvv[0], 1, numnum);
    *sv = a_std (&vvv[0], 1, numnum, *mv);
    *mV = a_mean(&VVV[0], 1, numnum);
    *sV = a_std (&VVV[0], 1, numnum, *mV);

    for(int i=1; i<=numnum; i++) {
       vvv[i]= (Tw) ((vvv[i]-(*mv) ) / ((*sv)+REG_EPS) );
       VVV[i]= (Tw) ((VVV[i]-(*mV) ) / ((*sV)+REG_EPS) );
    }
    a_diff(&vvv[0], &VVV[0], &vvv[0], 1, numnum);
    a_abs (&vvv[0], &vvv[0], 1, numnum);
    *fit = a_mean(&vvv[0], 1, numnum);
  } else {
   // yikes the images don't overlap after scaling rotating and
   // translating - this is bad so set fit equal to a very big value
   *fit = 1.0e08;
  }

  *overlap = ((double) numok) / ((double) numtest);
}

template <typename Timg, typename Tw>
void evalfit(const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *MG, Tw **m, Tw **v,
             double *fit, double *mv, double *sv, double *mV, double *sV, double *overlap) {
  int numnum = 0;

  Timg **g = (Timg **) mg->data;
  int g_rows = mg->rows;
  int g_cols = mg->cols;
  Timg **G = (Timg **) MG->data;
  int G_rows = MG->rows;
  int G_cols = MG->cols;

  int temp[5];
  temp[1] = g_rows;
  temp[2] = g_cols;
  temp[3] = G_rows;
  temp[4] = G_cols;
  int mindim = a_min(temp, 1, 4);

  //define skip
  int skip = std::max<int>(1, round<int>(mindim/32.0));

  int numok = 0;
  int difference = 0;
  int numtest=0;
  int ibot = round<int>(g_rows*0.25);
  int itop = round<int>(g_rows*0.75);
  int jbot = round<int>(g_cols*0.25);
  int jtop = round<int>(g_cols*0.75);

  int vvv_rows = (int) (ceil( ((double)(itop-ibot))/((double) skip) +1.0 )*
             ceil( ((double)(jtop-jbot))/((double) skip) +1.0 ) );
  int VVV_rows = vvv_rows ;
  std::vector<Tw> vvv(vvv_rows+1);
  std::vector<Tw> VVV(VVV_rows+1);
  
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
    }
  } 

  if (numok>0) {

    //rescale
    *mv = a_mean(&vvv[0], 1, numnum);
    *sv = a_std (&vvv[0], 1, numnum, *mv);
    *mV = a_mean(&VVV[0], 1, numnum);
    *sV = a_std (&VVV[0], 1, numnum, *mV);

    for(int i=1; i<=numnum; i++) {
       vvv[i]= (Tw) ((vvv[i]-(*mv) ) / ((*sv)+REG_EPS) );
       VVV[i]= (Tw) ((VVV[i]-(*mV) ) / ((*sV)+REG_EPS) );
    }
    a_diff(&vvv[0], &VVV[0], &vvv[0], 1, numnum);
    a_abs (&vvv[0], &vvv[0], 1, numnum);
    *fit = a_mean(&vvv[0], 1, numnum);
  } else {
   // yikes the images don't overlap after scaling rotating and
   // translating - this is bad so set fit equal to a very big value
   *fit = 1.0e08;
  }
  
  *overlap = ((double) numok) / ((double) numtest);
}

#endif // REG_EVALUATE_FIT_RST
