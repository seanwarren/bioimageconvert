//---------------------------------------------------------------------------

#define _USE_MATH_DEFINES //to get the constant to work

#include <cmath>
#include <math.h>
#include "radon.h"

#include "xtypes.h"

//---------------------------------------------------------------------------
/*
pPtr -array of double- output column. a pre-allocated vector of the size 2*ceil(norm(size(I)-floor((size(I)-1)/2)-1))+3
iPtr -double *- input pixels
thetaPtr -array of double- array of the size numAngles (degrees)
numAngles -int- the number of theta angles to compute
*/
void radon(double *pPtr, double *iPtr, double *thetaPtr, int M, int N,
           int xOrigin, int yOrigin, int numAngles, int rFirst, int rSize)
{
    // Allocate space for the lookup tables
    double *yTable = new double[2*M]; // x- and y-coordinate tables
    double *xTable = new double[2*N];
    double *xCosTable = new double[2*N]; //tables for x*cos(angle) and y*sin(angle)
    double *ySinTable = new double[2*M];

    /* x- and y-coordinates are offset from pixel locations by 0.25 */
    /* spaced by intervals of 0.5. */

    /* We want bottom-to-top to be the positive y direction */
    yTable[2*M-1] = -yOrigin - 0.25;
    for (int k = 2*M-2; k >=0; k--)       
        yTable[k] = yTable[k+1] + 0.5;

    xTable[0] = -xOrigin - 0.25;
    for (int k = 1; k < 2*N; k++)
        xTable[k] = xTable[k-1] + 0.5;

    for (int k = 0; k < numAngles; k++) {
        double angle = thetaPtr[k]; // radian angle value
        angle = (angle*M_PI)/180;
        double *pr = pPtr + k*rSize;  // pointer to the top of the output column - points inside output array
        double cosine = cos(angle); //cosine and sine of current angle
        double sine = sin(angle);   

        /* Radon impulse response locus:  R = X*cos(angle) + Y*sin(angle) */
        /* Fill the X*cos table and the Y*sin table.  Incorporate the */
        /* origin offset into the X*cos table to save some adds later. */
        for (int p = 0; p < 2*N; p++)
            xCosTable[p] = xTable[p] * cosine - rFirst;
        for (int p = 0; p < 2*M; p++)
            ySinTable[p] = yTable[p] * sine;

        /* Remember that n and m will each change twice as fast as the */
        /* pixel pointer should change. */
        #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (2*N>BIM_OMP_FOR2)
        for (int n = 0; n < 2*N; n++) {
            double *pixelPtr = iPtr + (n/2)*M; // points inside input array
            for (int m=0; m<2*M; m++) {
                double pixel = *pixelPtr; // current pixel value
                if (pixel) {
                    pixel *= 0.25;                         // 1 flop/pixel
                    double rIdx = (xCosTable[n] + ySinTable[m]);  // 1 flop/pixel - r value offset from initial array element
                    int rLow = (int) rIdx;                     // 1 flop/pixel 
                    double pixelLow = pixel*(1 - rIdx + rLow);    // 3 flops/pixel - amount of pixel's mass to be assigned to 
                    pr[rLow++] += pixelLow;                // 1 flop/pixel 
                    pr[rLow] += pixel - pixelLow;          // 2 flops/pixel
                }
                if (m%2)
                    pixelPtr++;   
            }
        }
    }

    delete [] yTable;
    delete [] xTable;
    delete [] xCosTable;
    delete [] ySinTable;
}

/* vd_RadonTextures
just change the order of the vector
vec -pointer to double- a pre-allocated vector with 12 enteries.
//out = [in(1) in(2) in(3) in(10) in(11) in(12) in(4) in(5) in(6) in(7) in(8) in(9)];
*/
void vd_RadonTextures(double *vec)
{  double temp[12];
int a;
temp[0]=vec[0];
temp[1]=vec[1];
temp[2]=vec[2];
temp[3]=vec[9];
temp[4]=vec[10];
temp[5]=vec[11];
temp[6]=vec[3];
temp[7]=vec[4];
temp[8]=vec[5];
temp[9]=vec[6];
temp[10]=vec[7];
temp[11]=vec[8];
for (a=0;a<12;a++)
    vec[a]=temp[a];
}
