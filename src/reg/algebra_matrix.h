/*******************************************************************************
 Matrix Algebra - Some linear algebra and util functions
 
 IMPLEMENTATION 

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Some functions by: Chao Huang 

 History:
   07/14/2001 21:53:31 - First creation
   08/06/2001 19:30:31 - bug correction, mf_mult function
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*******************************************************************************/

#ifndef ALGEBRA_MATRIX_H
#define ALGEBRA_MATRIX_H

#include <cmath>
#include <cstdlib>

#include "algebra_array.h"

#ifndef DBL_EPSILON 
#define DBL_EPSILON 2.2204460492503131e-016
#endif // DBL_EPSILON

#ifndef SVD_TRUNC 
#define SVD_TRUNC 1e-6 // trunctuate singular values smaller than this	(relative to largest)
#endif // SVD_TRUNC

#ifndef REG_EPS 
#define REG_EPS 2.2204460492503131e-016
#endif // REG_EPS

#ifndef REG_PI 
#define REG_PI 3.1415926535897932384626433832795
#endif // REG_PI

inline double eps() { return DBL_EPSILON; }

template<typename T>
inline T sqr(T a) { return a*a; }

template <typename To, typename Ti>
inline To round( const Ti &x ) {
  return (To) floor(x + 0.5);
}

template <typename T>
inline T rrand() {
  return ((T) rand()) / RAND_MAX;
}

template <typename T>
inline T log2(T n) {
  return (T) (log((double)n)/log(2.0));
}


//****************************************************************************
// basic Matrix array operations
//****************************************************************************
template <typename T>
T **m_init(int rows, int cols);

template <typename T>
int m_free(T **m);

// both matrix have to be allocated, copy g1->g2
template <typename T>
int m_copy(T **g1, T **g2, int rows, int cols);

//****************************************************************************
// some util functions
//****************************************************************************

template <typename T>
void m_zeros(T **g, int rows, int cols);

template <typename T>
void m_ones(T **g, int rows, int cols);

template <typename T, typename Tpoint>
Tpoint m_max_point(T **g, int rows, int cols);

/*
This finds the max of elements of an matrix and a scalar

INPUT :     
     input    : matrix
     scalar   : scalar value
     row     : rows of matrix
     col     : cols of matrix
  RETURN
     output   : matrix containing output
*/
template <typename T>
int m_max_scalar(T **input, T **output, int rows, int cols, double scalar);

//This function finds the mean of an matrix 
// same as Matlab by rows
template <typename T>
int m_mean( T **g, int rows, int cols, T *m, int beg);

//This function finds the mean of an matrix
// full matrix
template <typename T>
double m_mean( T **g, int rows, int cols);

//this function finds the Standard deviation of an matrix
//It normalises by N-1, where N is the number of elements of matrix
template <typename T>
int m_std( T **g, int rows, int cols, T *s, int beg);

template <typename T>
double m_std( T **g, int rows, int cols);

template <typename T>
double m_std( T **g, int rows, int cols, double mean);

/*
this function multiplies two matrices and returns the value
  in the third one

INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
     
RETUNS
     mat3     : 2-D matrix of dimension row1 X col2 
*/
template <typename T>
int m_mult(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3);

/*this function subtracts two matrices and returns the value
  in the third one
   INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
   OUTPUT 
     mat3     : 2-D matrix3
     row3     : number of rows of matrix3  
     col3     : number of cols of matrix3
*/
template <typename T>
int m_diff(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3);

template <typename T>
int m_summ(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3);

template <typename T>
int m_abs(T **mat1, int row1, int col1);

template <typename T>
int m_sum(T **g, int rows, int cols, T *m);

/*this finds the min element in an matrix
  INPUT :     
     input    : matrix
     len     : length of matrix
  RETURN
     output   : smallest element in input 
*/
template <typename T> 
int m_min(T **g, int rows, int cols, T *m, int beg);
 
template <typename T> 
int m_max(T **g, int rows, int cols, T *m, int beg);

template <typename T> 
T m_min(T **g, int rows, int cols);
 
template <typename T> 
T m_max(T **g, int rows, int cols);

// !!!! t rows = g cols and versa
template <typename T> 
int m_transpose(T **g, int rows, int cols, T **t);

// transpose of quadratic matrix
template <typename T> 
int m_transpose(T **g, int rows);

//void mf_printfile(char *filename, char* type, char* msg, T **g, int rows, int cols);

/*
this function adds two matrices and returns the value
  in the third one
INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
OUTPUT 
     mat3     : 2-D matrix3
*/
template <typename T> 
int m_add(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3);

template <typename T> 
int m_floor(T **g, T **h, int row, int col);

template <typename T> 
double m_norm(T **mat, int row, int col); 





//****************************************************************************
// basic Matrix array operations
//****************************************************************************

template <typename T>
T **m_init(int rows, int cols) {
  // allocate pointers to rows
	T **m = (T **) malloc( (rows+1)*sizeof(T*) );
	if (!m) return NULL;

	// allocate rows of 0 and set pointers to them
  m[0] = (T *) malloc( ((rows+1)*(cols+1))*sizeof(T) );
	if (!m[0]) return NULL;

	for(int i=1; i<=rows; i++) m[i]=m[i-1]+cols;

	// return pointer to array of pointers to rows
  return m;
}

template <typename T>
int m_free(T **m) {
  if (!m) return 1;
	free(m[0]);
	free(m);
  return 0;
}

// both matrix have to be allocated, copy g1->g2
template <typename T>
int m_copy(T **g1, T **g2, int rows, int cols) {
  if ( (g1 == NULL) || (g2 == NULL) ) return 1;
  for (int i=1; i<=rows; i++)
    for (int j=1; j<=cols; j++)
      g2[i][j] = g1[i][j];
  return 0;
}

//****************************************************************************
// some Matlab functions
//****************************************************************************

template <typename T>
void m_zeros(T **g, int rows, int cols) {
   for(int i=1; i<=rows; i++)
     for(int j=1; j<=cols; j++)
        g[i][j]=0.0;
}

template <typename T>
void m_ones(T **g, int rows, int cols) {
   for(int i=1;i<=rows;i++)
     for(int j=1;j<=cols;j++)
        g[i][j]=1.0;
}

template <typename T, typename Tpoint>
Tpoint m_max_point(T **g, int rows, int cols) {
   T max = g[1][1];
   Tpoint xy(-1, -1);
   for (int i=1; i<=rows; i++)
     for (int j=1; j<=cols; j++)
        if (max < g[i][j]) {
          max = g[i][j];
          xy.x = (typename Tpoint::data_type) i;
          xy.y = (typename Tpoint::data_type) j;
        }
   return xy;
}


/*
This finds the max of elements of an matrix and a scalar

INPUT :     
     input    : matrix
     scalar   : scalar value
     row     : rows of matrix
     col     : cols of matrix
  RETURN
     output   : matrix containing output
*/
template <typename T>
int m_max_scalar(T **input, T **output, int rows, int cols, double scalar) {
   if (input == NULL || output == NULL) return 1;
   for(int i=1; i<=rows; i++)
     for(int j=1; j<=cols; j++)
       output[i][j] = std::max<T>( (T)scalar, input[i][j]);
   return 0;
}


//This function finds the mean of an matrix 
// same as Matlab by rows
template <typename T>
int m_mean( T **g, int rows, int cols, T *m, int beg) {
  for (int j=1; j<=cols; j++) {
    double sm = 0.0;
    int jj = j+beg-1;
    for (int i=1; i<=rows; i++) 
      sm += g[i][j];
    m[jj] = (T) (sm /(T)rows);
    ++jj;
  }
  return 0;
}

//This function finds the mean of an matrix
// full matrix
template <typename T>
double m_mean( T **g, int rows, int cols) {
  double avg=0.0;
  for (int j=1; j<=cols; j++)
    for (int i=1; i<=rows; i++) 
       avg = avg + g[i][j];

  return (avg / ((double) rows*cols));
}

//this function finds the Standard deviation of an matrix
//It normalises by N-1, where N is the number of elements of matrix
template <typename T>
int m_std( T **g, int rows, int cols, T *s, int beg) {
  std::vector<T> m(cols+1);
  m_mean(g, rows, cols, &m[0], beg);
  
  for (int j=1; j<=cols; j++) {
    double ss = 0.0;
    int jj = j+beg-1;
    for (int i=1; i<=rows; i++) 
      ss += (g[i][j]-m[jj])*(g[i][j]-m[jj]);
    s[jj] = (T) sqrt( ss/(rows-1) );
    ++jj;
  }
  return 0;
}

template <typename T>
double m_std( T **g, int rows, int cols) {
  double std = 0.0;
  double mean = m_mean(g, rows, cols);
  for (int j=1; j<=cols; j++)
    for (int i=1; i<=rows; i++) 
      std = std + ( (((double) g[i][j])-mean)*(((double) g[i][j])-mean) );
  return sqrt( std/((double) rows*cols-1) );
}

template <typename T>
double m_std( T **g, int rows, int cols, double mean) {
  double std = 0.0;
  for (int j=1; j<=cols; j++)
    for (int i=1; i<=rows; i++) 
      std = std + ( (((double) g[i][j])-mean)*(((double) g[i][j])-mean) );
  return sqrt( std/((double) rows*cols-1) );
}


/*
this function multiplies two matrices and returns the value
  in the third one

INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
     
RETUNS
     mat3     : 2-D matrix of dimension row1 X col2 
*/
template <typename T>
int m_mult(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3) {
  if ( (col1 != row2) || (mat1 == NULL || mat2 == NULL || mat3 == NULL) ) return 1;
  for (int i=1; i<=row1; i++)
  for (int j=1; j<=col2; j++) {
    double temp = 0.0;
    for (int k = 1; k <= col1; k++) temp += mat1[i][k]*mat2[k][j];
    mat3[i][j] = (T) temp; 
  }

  return 0;
}     

/*this function subtracts two matrices and returns the value
  in the third one*/

/* INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
   OUTPUT 
     mat3     : 2-D matrix3
     row3     : number of rows of matrix3  
     col3     : number of cols of matrix3
     
*/
template <typename T>
int m_diff(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3) {
  if ( (col1 != col2 || row1 != row2) ||
       (mat1 == NULL || mat2 == NULL || mat3 == NULL) ) return 1;

  for (int i=1; i<=row1; i++)
    for (int j=1; j<=col1; j++)
      mat3[i][j] = mat1[i][j] - mat2[i][j]; 

  return 0;
}     

template <typename T>
int m_summ(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3) {

  if ( (col1 != col2 || row1 != row2) ||
       (mat1 == NULL || mat2 == NULL || mat3 == NULL) ) return 1;

  for (int i=1; i<=row1; i++)
    for (int j=1; j<=col1; j++)
      mat3[i][j] = mat1[i][j] + mat2[i][j]; 

  return 0;
} 

template <typename T>
int m_abs(T **mat1, int row1, int col1) {
  if (mat1 == NULL) return 1;
  for (int i=1; i<=row1; i++)
    for (int j=1; j<=col1; j++)
      mat1[i][j] = (T) fabs(mat1[i][j]); 
  return 0;
} 

template <typename T>
int m_sum(T **g, int rows, int cols, T *m) {
  if (g == NULL || m == NULL) return 1;
  for (int j=1; j<=cols; j++) {
    m[j-1] = 0.0;
    for (int i=1; i<=rows; i++) m[j-1] = m[j-1] + g[i][j];
  }
  return 0;
}

/*this finds the min element in an matrix */ 

/*INPUT :     
     input    : matrix
     len     : length of matrix
  RETURN
     output   : smallest element in input 
*/
template <typename T> 
int m_min(T **g, int rows, int cols, T *m, int beg) {
  if (g == NULL || m == NULL) return 1;
  for (int j=1; j<=cols; j++) {
    T *t = &m[j+beg-1];
    *t = g[1][j];
    for (int i=1; i<=rows; i++) 
      if (*t > g[i][j]) *t = g[i][j];
  }
  return 0;
}

template <typename T> 
int m_max(T **g, int rows, int cols, T *m, int beg) {
  if (g == NULL || m == NULL) return 1;
  for (int j=1; j<=cols; j++) {
    T *t = &m[j+beg-1];
    *t = g[1][j];
    for (int i=1; i<=rows; i++) 
      if (*t < g[i][j]) *t = g[i][j];
  }
  return 0;
}

template <typename T> 
T m_min(T **g, int rows, int cols) {
  T minval = g[1][1];
  for (int j=1; j<=cols; j++)
    for (int i=1; i<=rows; i++) 
      if (minval > g[i][j]) minval = g[i][j];
  return minval;
}

template <typename T> 
T m_max(T **g, int rows, int cols) {
  T minval = g[1][1];
  for (int j=1; j<=cols; j++)
    for (int i=1; i<=rows; i++) 
      if (minval < g[i][j]) minval = g[i][j];
  return minval;
}

// !!!! t rows = g cols and versa
template <typename T> 
int m_transpose(T **g, int rows, int cols, T **t) {
  if (g == NULL) return 1;
  for (int i=1; i<=rows; i++)
    for (int j=1; j<=cols; j++)
      t[j][i] = g[i][j];
  return 0;
}

// transpose of quadratic matrix
template <typename T> 
int m_transpose(T **g, int rows) {
  if (g == NULL) return 1;
  for (int i=1; i<=rows; i++)
    for (int j=1; j<=rows; j++) {
      swap_values(g[j][i], g[i][j]); 
    }
  return 0;
}

/*
void mf_printfile(char *filename, char* type, char* msg, 
                  T **g, int rows, int cols)
{
  FILE *out;
  int i,j;

  out = fopen(filename, type); 

  fprintf(out, "\n%s : [%d*%d]\n", msg, rows, cols);
  for (i=1; i<=rows; i++)
  {
    for (j=1; j<=cols; j++)
      fprintf(out, " %7.4f ", g[i][j]);
    fprintf(out, "\n");
  }
  fclose(out);
}*/


/*
this function adds two matrices and returns the value
  in the third one

INPUT 
     mat1     : 2-D matrix1 
     mat2     : 2-D matrix2
     row1     : number of rows of matrix1  
     row2     : number of rows of matrix2
     col1     : number of cols of matrix1
     col2     : number of rows of matrix2
OUTPUT 
     mat3     : 2-D matrix3
*/
template <typename T> 
int m_add(T **mat1, T **mat2, int row1, int col1, int row2, int col2, T **mat3) {
  if (mat1 == NULL || mat2 == NULL || mat3 == NULL) return 1;
  if (col1 != col2 || row1 != row2) return 2;
  for (int i = 1; i <= row1; i++)
    for (int j = 1; j <= col1; j++)
      mat3[i][j] = mat1[i][j] + mat2[i][j]; 
  return 0;
}   

template <typename T> 
int m_floor(T **g, T **h, int row, int col) {
  for (int i=1; i<=row; i++)
    for(int j=1; j<=col; j++)
      h[i][j] = (T) floor(g[i][j]);
  return 0;
}

template <typename T> 
double m_norm(T **mat, int row, int col) {
   double norm = 0.0;
   //norm = sqrt(squares of elements in the matrix)
   for(int i = 1; i <=row; i++)
      for(int j=1;j<=col;j++)
         norm += mat[i][j]*mat[i][j];
   return sqrt(norm);
}

/*****************************************************************************
 PINV
 ****************************************************************************/

template <typename T> 
T pythag(T a, T b) {
	T absa = (T) fabs(a);
	T absb = (T) fabs(b);
	if (absa > absb) 
    return absa*((T) sqrt(1.0+sqr(absb/absa)));
	else 
    return ((T)(absb == 0.0 ? 0.0 : absb*((T) sqrt(1.0+sqr(absa/absb) )) ));
}

template <typename T>
T sign(T a, T b) {
  if (b >= 0.0) 
    return (T) fabs(a);
  else 
    return (T) -fabs(a);
}

template <typename T>
int svdcmp(T **a, int m, int n, T *w, T **v) {

  int flag, i, its, j, jj, k, l=0, nm=0;
	T anorm,c,f,g,h,s,scale,x,y,z;

  std::vector<T> rv1(n+1);

	g=scale=anorm=0.0;
	for (i=1;i<=n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) {
			for (k=i;k<=m;k++) scale += (T) fabs(a[k][i]);
			if (scale) {
				for (k=i;k<=m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -sign((T) sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) {
			for (k=l;k<=n;k++) scale += (T) fabs(a[i][k]);
			if (scale) {
				for (k=l;k<=n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -sign((T) sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
    anorm=std::max<T>(anorm, (T) (fabs(w[i])+fabs(rv1[i])) ); // dima
	}
	for (i=n;i>=1;i--) {
		if (i < n) {
			if (g) {
				for (j=l;j<=n;j++)
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=std::min<int>(m,n);i>=1;i--) {
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) {
			g=(T) 1.0/g;
			for (j=l;j<=n;j++) {
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) {
		for (its=1;its<=30;its++) {
			flag=1;
			for (l=k;l>=1;l--) {
				nm=l-1;
				if ((T)(fabs(rv1[l])+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((T)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((T)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=(T)1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) {
				if (z < 0.0) {
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) return 2;
			x=w[l];
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/((T)2.0*h*y);
			g=pythag<T>(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+sign(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z;
				if (z) {
					z=(T)1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}

  return 0;
}


/*****************************************************************************
 *
 * (c) 1997 Robert Oostenveld, available under GNU licence
 *
 * pseudoinverse of a square or nonsquare matrix
 *   Multiplication of pinv(A) with A gives the identity matrix.
 *   This function replaces the matrix A with the TRANSPOSE of the 
 *   pseudoinverse matrix.
 *
 ****************************************************************************/

template <typename T> 
int pinv(T **A, int rows, int cols, T **I) {

  T **U = m_init<T>(rows, cols);
  m_copy(A, U, rows, cols);
  T **V = m_init<T>(cols, cols);
  std::vector<T> s(cols+1);

  long res = svdcmp(U, rows, cols, &s[0], V);
  
  if (res != 2) {
    // calculate the pseudoinverse matrix                
    // remove the near singular values from the vector s 
    T maxval = a_max(&s[0], 1, cols);

    for (int i=1; i<=cols; i++)
    for (int j=1; j<=rows; j++) {
	    A[j][i] = 0;
	    for (int k=1; k<=cols; k++)
		    A[j][i] += V[i][k] * U[j][k] * (s[k]/maxval < SVD_TRUNC ? 0 : 1/s[k]);
    }
  }
  m_transpose(A, rows, cols, I);
  m_free(U);
  m_free(V);
  return(res);
}



#endif // ALGEBRA_MATRIX_H
