/*****************************************************************************
  Some stuff from numerical recipies, sligtly updated
*****************************************************************************/

#ifndef NUMERICAL_RECIPIES_H
#define NUMERICAL_RECIPIES_H

#include <vector>

#define TINY 1.0e-20;

template< typename T >
void ludcmp(T** a, int n, int* indx, T* d) {
  int i,imax=1,j,k;
  T big,dum,sum,temp;
  
  std::vector<T> vv(n+1);

  *d=1.0;
  for (i=1;i<=n;i++) {
    big=0.0;
    for (j=1;j<=n;j++)
      if ((temp = (T) fabs(a[i][j])) > big) big=temp;
    //if (big == 0.0) printf("\nSingular matrix in routine LUDCMP");
    if (big!=0.0) 
      vv[i]=(T) (1.0/big);
    else
      vv[i]= 0.0;
  }
  for (j=1;j<=n;j++) {
    for (i=1;i<j;i++) {
      sum=a[i][j];
      for (k=1;k<i;k++) sum -= a[i][k]*a[k][j];
      a[i][j]=sum;
    }
    big=0.0;
    for (i=j;i<=n;i++) {
      sum=a[i][j];
      for (k=1;k<j;k++)
        sum -= a[i][k]*a[k][j];
      a[i][j]=sum;
      if ( (dum= (T) (vv[i]*fabs(sum))) >= big) {
        big=dum;
        imax=i;
      }
    }
    if (j != imax) {
      for (k=1;k<=n;k++) {
        dum=a[imax][k];
        a[imax][k]=a[j][k];
        a[j][k]=dum;
      }
      *d = -(*d);
      vv[imax]=vv[j];
    }
    indx[j]=imax;
    if (a[j][j] == 0.0) a[j][j]=(T)TINY;
    if (j != n) {
      dum=(T)(1.0/(a[j][j]));
      for (i=j+1;i<=n;i++) a[i][j] *= dum;
    }
  }
}

#undef TINY

template< typename T >
void lubksb(T** a, int n, int* indx, T* b) {
  int i,ii=0,ip,j;
  T sum;

  for (i=1;i<=n;i++) {
    ip=indx[i];
    sum=b[ip];
    b[ip]=b[i];
    if (ii)
      for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
    else if (sum) ii=i;
    b[i]=sum;
  }
  for (i=n;i>=1;i--) {
    sum=b[i];
    for (j=i+1;j<=n;j++) sum -= a[i][j]*b[j];
    b[i]=sum/a[i][i];
  }
}

#endif // NUMERICAL_RECIPIES_H
