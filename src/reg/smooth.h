/*****************************************************************************
 Smoothing functions

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   07/14/2001 00:26:02 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatmen

 Ver : 2
*****************************************************************************/

#ifndef REG_SMOOTH_H
#define REG_SMOOTH_H

template <typename T>
void smooth( reg::Matrix<T> *gm, int num )
// Gaussian smoothing for the matrix g.
// Input:
//   g = image
//   num = number of Gaussian smoothings
// Output: 
//   w = image 
{ 

  //w=g;
  reg::Matrix<T> wm;

  //[nr,nc]=size(g);
  int nr = gm->rows;
  int nc = gm->cols;

  for (int n=1; n<=num; n++) {
    wm.fromMatrix(gm);
    T **w = (T **) gm->data;
    T **g = (T **) wm.data;

    // Corners.
    w[1][1]   = (g[1][1] + g[2][2] + g[2][1] + g[1][2])/(T)4.0;
    w[1][nc]  = (g[1][nc]+g[2][nc]+g[1][nc-1]+g[2][nc])/(T)4.0;
    w[nr][1]  = (g[nr][1]+g[nr-1][1]+g[nr-1][1]+g[nr][2])/(T)4.0;
    w[nr][nc] = (g[nr][nc]+g[nr-1][nc-1]+g[nr-1][nc]+g[nr][nc-1])/(T)4.0;

    // Edges.
    //i=2:nr-1;
    #pragma omp parallel for default(shared)
    for (int i=2; i<nr; i++) {
      w[i][1]  = (g[i+1][1]+g[i][1]+g[i][2]+g[i-1][1])/(T)4.0;
      w[i][nc] = (g[i+1][nc]+g[i][nc]+g[i][nc-1]+g[i-1][nc])/(T)4.0;

      // Top and Bottom.
      //j=2:nc-1;
      for (int j=2; j<nc; j++) {
        w[1][j]  = (g[1][j+1]+g[1][j]+g[2][j]+g[1][j-1])/(T)4.0;
        w[nr][j] = (g[nr][j+1]+g[nr][j]+g[nr-1][j]+g[nr][j-1])/(T)4.0;

        // Central smoothing.
        w[i][j] = (g[i-1][j]+4*g[i][j]+g[i+1][j]+g[i][j-1]+g[i][j+1])/(T)8.0;
      }

    }
  }
}

//****************************************************************************
// arrays, DIMINGraph functions init array M[1] of rows*cols
//****************************************************************************
template <typename T>
int smooth(T *g, int g_row, int g_col, int num) {

  std::vector<T> w(g_row*g_col);
  for (int k=1; k<=num; k++) {
		/* corners */
		w[0]=(g[0]+g[1]+g[g_col]+g[g_col+1])/4;
		w[g_col-1]=(g[g_col-2]+g[g_col-1]+g[2*g_col-2]+g[2*g_col-1])/4;
		w[(g_row-1)*g_col]=(g[(g_row-1)*g_col]+g[(g_row-1)*g_col+1]+g[(g_row-2)*g_col]+g[(g_row-2)*g_col+1])/4;
		w[g_row*g_col-1]=(g[g_row*g_col-1]+g[g_row*g_col-2]+g[(g_row-1)*g_col-1]+g[(g_row-1)*g_col-2])/4;

		/* sides */
        #pragma omp parallel for default(shared)
		for (int j=1; j<g_row-1;j++) {
			w[j*g_col]=(g[j*g_col]+g[(j-1)*g_col]+g[(j+1)*g_col]+g[j*g_col+1])/4;
			w[(j+1)*g_col-1]=(g[(j+1)*g_col-1]+g[j*g_col-1]+g[(j+2)*g_col-1]+g[(j+1)*g_col-2])/4;
		}
		
		/* top & bottom */	
        #pragma omp parallel for default(shared)
		for (int j=1; j<g_col-1;j++) {
			w[j]=(g[j]+g[j-1]+g[j+1]+g[j+g_col])/4;
			w[(g_row-1)*g_col+j]=(g[(g_row-1)*g_col+j]+g[(g_row-1)*g_col+j-1]+g[(g_row-1)*g_col+j+1]+g[(g_row-2)*g_col+j])/4;
		}
		
		/* center */
        #pragma omp parallel for default(shared)
		for (int i=1; i<g_row-1; i++)
			for (int j=1; j<g_col-1; j++)
				w[i*g_col +j]=(4*g[i*g_col +j]+g[i*g_col +j-1]+g[i*g_col +j+1]+g[(i-1)*g_col +j]+g[(i+1)*g_col +j])/8;
		
		/* transfer */
		
		for (unsigned int i=0; i<(g_row*g_col); i++)
			g[i]=w[i];
	}
	return 0;
}

#endif // REG_SMOOTH_H
