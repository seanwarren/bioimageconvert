/*****************************************************************************
  Original Matlab code by Charles Kenney 
  First converted by Chao Huang, 2001
  Posterior modif. Dima V. Fedorov, 2001

INPUT:
  mg      : 2-D array
  mG      : 2-D array

OUTPUT:
  fit, fitsigma : pointer to double

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/02/2001 18:40:30 - First creation
   08/15/2001 22:34:38 - Bug corrected, incorrect parameters on evalfit call
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_FIT_ERROR
#define REG_FIT_ERROR

/*
 now we have do some random
 perturbations of the parameters and see the
 effect on the fit between g and G
 we want to restrict the perturbations so that
 the maximum index error is less than maxindexerror

 as a preliminary we must make a rough 
 estimate of the size of the allowable perturbation
*/

#include <cmath>

template< typename Tw>
double count_error(Tw **m, Tw **v, Tw **mm, Tw **vv, Tw **ij) {

  Tw **IJ = m_init<Tw>(2, 1);
  Tw **IIJJ = m_init<Tw>(2, 1);

  //IJ=m*ij+v; IIJJ=mm*ij+vv; err=norm(IJ-IIJJ);
  m_mult(m, ij, 2,2,2,1, IJ);
  m_add (IJ, v, 2,1,2,1, IJ);
  m_mult(mm,ij,2,2,2,1,IIJJ);
  m_add (IIJJ,vv,2,1,2,1,IIJJ);
  m_diff(IJ,IIJJ,2,1,2,1,IJ);
  
  double err = m_norm(IJ,2,1);
  m_free(IJ);
  m_free(IIJJ);

  return err;
}

template<typename Timg, typename Tw>
void fiterror(const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *mG,  
              Tw **m, Tw **v, int num, double maxindexerror,
              double *fit, double *fitsigma)
{
   //double dr,da,ddx,ddy,rr,aa,ddxx,ddyy,cc,ss;

   double r, a, dx, dy;
   aff2rst(m, v, r, a, dx, dy);

   int rows = (int) min(mg->rows, mG->rows);
   int cols = (int) min(mg->cols, mG->cols);

   Tw **mm = m_init<Tw>(2, 2);
   Tw **vv = m_init<Tw>(2, 1);
   Tw **ij = m_init<Tw>(2, 1);
   
   double *FIT = (double *) calloc(num+1, sizeof(double));
   int N = std::max<int>(rows, cols);

   double DelR = 4.0*maxindexerror/((double) N);
   double DelA = 12.0*maxindexerror/((double) N);
   double DelX = 2.0*maxindexerror;
   double DelY = 2.0*maxindexerror;
   
   /* We sample uniformly inside the cube
      [-DelR,DelR]x[-DelA,DelA]x[-DelX,DelX]x[-DelY,DelY]
      and use the allowable perts
   */
   
   int numok=0;
   for(int k=1; k<=20000*num; k++) {
       double dr  = (rrand<double>()-0.5)*2.0*DelR;
       double da  = (rrand<double>()-0.5)*2.0*DelA;
       double ddx = (rrand<double>()-0.5)*2.0*DelX;
       double ddy = (rrand<double>()-0.5)*2.0*DelY;

       // this perturbation may be too big!
       // we got to check this out
       double rr = r+dr;
       double aa = a+da;
       double ddxx = dx+ddx;
       double ddyy = dy+ddy;

       double cc = cos(aa*REG_PI/180.0);
       double ss = sin(aa*REG_PI/180.0);

       mm[1][1] = (rr*cc);
       mm[1][2] =  (rr*ss);
       mm[2][1] =  (-rr*ss);
       mm[2][2] =  (rr*cc);

       vv[1][1] =  ddxx;
       vv[2][1] =  ddyy;
    
       bool iflag = false;
 
       //check 1
       ij[1][1]=1;
       ij[2][1]=1;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=1;
       ij[2][1]=cols;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=rows;
       ij[2][1]=1;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=rows;
       ij[2][1]=cols;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       if (!iflag) {
          double FITT;
          double overlap;
          double mv,mV,sv,sV;
          evalfit(mg, mG, rr, cc, ss, ddxx, ddyy, &FITT, &mv, &sv, &mV, &sV, &overlap);
          if (FITT<1000) {
             numok++;
             FIT[numok]=FITT;
          }
       }
       if (numok==num) break;
   }

   *fit = a_mean(FIT, 1, numok);
   *fitsigma = a_std(FIT, 1, numok, *fit);

   m_free(mm);
   m_free(vv);
   m_free(ij);

   free(FIT);
}

template <typename Timg, typename Tw>
void fiterror(const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *mG,  
              double r, double a, double dx, double dy, int num, double maxindexerror,
              double *fit, double *fitsigma) {

   int rows = std::min<int>(mg->rows, mG->rows);
   int cols = std::min<int>(mg->cols, mG->cols);

   Tw **m  = m_init<Tw>(2, 2);
   Tw **mm = m_init<Tw>(2, 2);
   Tw **v  = m_init<Tw>(2, 1);
   Tw **vv = m_init<Tw>(2, 1);
   Tw **ij = m_init<Tw>(2, 1);
   
   double *FIT = (double *) calloc(num+1, sizeof(double));

   double c = cos(a*REG_PI/180.0);
   double s = sin(a*REG_PI/180.0);

   m[1][1] = (Tw) (r*c);
   m[1][2] = (Tw) (r*s);
   m[2][1] = (Tw) (-r*s);
   m[2][2] = (Tw) (r*c);

   v[1][1] = (Tw) dx;
   v[2][1] = (Tw) dy;

   int N = std::max<int>(rows, cols);

   double DelR = 4.0*maxindexerror/((double) N);
   double DelA = 12.0*maxindexerror/((double) N);
   double DelX = 2.0*maxindexerror;
   double DelY = 2.0*maxindexerror;
   
   /* We sample uniformly inside the cube
      [-DelR,DelR]x[-DelA,DelA]x[-DelX,DelX]x[-DelY,DelY]
      and use the allowable perts
   */
   
   int numok=0;
   for(int k=1; k<=20000*num; k++) {
       double dr  =(rrand<double>()-0.5)*2.0*DelR; 
       double da  =(rrand<double>()-0.5)*2.0*DelA;
       double ddx =(rrand<double>()-0.5)*2.0*DelX;
       double ddy =(rrand<double>()-0.5)*2.0*DelY;

       // this perturbation may be too big!
       // we got to check this out
       double rr = r+dr;
       double aa = a+da;
       double ddxx = dx+ddx;
       double ddyy = dy+ddy;

       double cc = cos(aa*REG_PI/180.0);
       double ss = sin(aa*REG_PI/180.0);

       mm[1][1] = (Tw) (rr*cc);
       mm[1][2] = (Tw) (rr*ss);
       mm[2][1] = (Tw) (-rr*ss);
       mm[2][2] = (Tw) (rr*cc);

       vv[1][1] = (Tw) ddxx;
       vv[2][1] = (Tw) ddyy;
    
       bool iflag = false;
 
       //check 1
       ij[1][1]=1;
       ij[2][1]=1;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=1;
       ij[2][1]=(Tw) cols;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=(Tw) rows;
       ij[2][1]=1;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       ij[1][1]=(Tw) rows;
       ij[2][1]=(Tw) cols;
       if ( count_error(m, v, mm, vv, ij) > maxindexerror ) iflag=true;

       if (!iflag) {
          double FITT;
          double overlap;
          double mv,mV,sv,sV;
          evalfit<Timg, Tw>(mg, mG, rr, cc, ss, ddxx, ddyy, &FITT, &mv, &sv, &mV, &sV, &overlap);
          if (FITT<1000) {
             numok++;
             FIT[numok]=FITT;
          }
       }
       if (numok==num) break;
   }

   *fit = a_mean(FIT, 1, numok);
   *fitsigma = a_std(FIT, 1, numok, *fit);

   m_free(m);
   m_free(mm);
   m_free(v);
   m_free(vv);
   m_free(ij);

   free(FIT);
}

#endif // REG_FIT_ERROR
