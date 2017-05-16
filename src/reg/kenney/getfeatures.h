/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

 This function differs from Matlab by processing only one image at a time
 and not for both as in matlab.

INPUT:
  mg   - 2-D array
  pA   - pointer to TPointA

OUTPUT:
  Matw - 2-D array, do not init it before!!!

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   08/10/2001 15:06:32 - use of TPointA
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_GET_FEATURES
#define REG_GET_FEATURES

template< typename Tw, typename Timg, typename Tpoint >
int getfeatures(const reg::Matrix<Timg> *mg, std::deque< reg::Point<Tpoint> > *points, int wind, reg::Matrix<Timg> *mwout)
// get the windows for the tiepoints in g
// these windows are the values of g around the
// tiepoint but rotated so the gradient points
// in the direction (1,0)

// normalize each window by subtracting its mean
// and dividing by its standard deviation
// note we don't normalize the last two rows since these rows
// contain the tiepoint window rotation information (c,s) (see getwindows for more details)

// wout is a matrix: the jth column of wout
// is a stacked version of the window about the
// jth tiepoint (x(j),y(j))
// the window is rotated so its gradient points downward
{
  int rg = mg->rows;
  int cg = mg->cols;
  Timg **g = (Timg **) mg->data;
  
  int wind_size = 2*wind+1;
  int wsize = wind_size * wind_size;

  // we'll store rotation and radiometry normalized windows + 2 rotation parameters for each point
  mwout->init(wsize+2, (int)points->size());
  Timg **wout = mwout->data;
  
  #pragma omp parallel for default(shared)
  for (int p=1; p<=points->size(); ++p) {
    Timg **wnew = m_init<Timg>(wind_size, wind_size);
    int i = (int) (*points)[p-1].y;
    int j = (int) (*points)[p-1].x;
    
    // get the center of intensity vector at the point (i,j) 
    double wx=0.0, wy=0.0;
    for (int t1 = j-wind; t1<=j+wind; t1++) wx = wx + (g[i+1][t1]-g[i-1][t1]);
    for (int t2 = i-wind; t2<=i+wind; t2++) wy = wy + (g[t2][j+1]-g[t2][j-1]);

    int k2 = (int) floor(wind/2.0);

    // use a circular window to estimate the centroid of intensity
    for (int k=2; k<=k2; k++) {
      double tx = 0, ty = 0;
      for (int t1 = j-wind; t1<=j+wind; t1++) tx = tx + g[i+k][t1]-g[i-k][t1];
      wx=wx+k*tx;
      for (int t2 = i-wind; t2<=i+wind; t2++) ty = ty + g[t2][j+k]-g[t2][j-k];
      wy=wy+k*ty;
    }
    for (int k=k2+1; k<=wind; k++) {
      int kk = k-k2;
      double tx = 0, ty = 0;
      for (int t1=j-wind+kk; t1<=j+wind-kk; t1++) tx=tx+g[i+k][t1]-g[i-k][t1];
      wx=wx+k*tx;
      for (int t1=i-wind+kk; t1<=i+wind-kk; t1++) ty=ty+g[t1][j+k]-g[t1][j-k];
      wy=wy+k*ty;
    }

    double norm = sqrt(wx*wx+wy*wy) + REG_EPS;
    double c = wx / norm;
    double s = wy / norm;

    // now set up the rotated window so that the 
    // center of intensity vector points in the direction (1,0)
    for (int I=-wind; I<=wind; I++)
    for (int J=-wind; J<=wind; J++) {
      double ax = (c*I-s*J);
      double ay = (s*I+c*J);
      int ii = (int) floor(ax);
      int jj = (int) floor(ay);
      ax = ax - ii;
      ay = ay - jj;

      // make sure we don't go outside the image!
      int is=i+ii;
      if (is<1) is=1;
      if (is>rg-1) is=rg-1;
      int js=j+jj;
      if (js<1) js=1;
      if (js>cg-1) js=cg-1;

      double ggg=(1-ax)*(1-ay)*g[is][js];
      ggg=ggg+(1-ax)*(ay)*g[is][js+1];
      ggg=ggg+(ax)*(1-ay)*g[is+1][js];
      ggg=ggg+(ax)*(ay)*g[is+1][js+1];

      wnew[I+wind+1][J+wind+1] = (Timg) ggg;
    }
    
    // compute mean and std to normalize the feature vector
    double mw = m_mean(wnew, wind_size, wind_size);
    double sw = m_std(wnew, wind_size, wind_size, mw);

    int t1=1;
    for (int ix=1; ix<=wind_size; ix++)
    for (int iy=1; iy<=wind_size; iy++) {
      wout[t1][p] = (Timg)((wnew[ix][iy]-mw)/(sw+REG_EPS));
      t1++;
    }

    // include the rotation angle as feature info
    wout[wsize+1][p] = (Timg) c;
    wout[wsize+2][p] = (Timg) s;
    m_free(wnew);
  }
  
  return 0;
}

#endif // REG_GET_FEATURES
