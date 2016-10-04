imgcnv -verbose 2 -i A02.jpg -o out1.tif -t tiff -superpixels 16,0.2

set OMP_NUM_THREADS=1
imgcnv -verbose 2 -i A02.jpg -o out2.tif -t tiff -superpixels 16,0.2

REM SLIC superpixels took 33.723999 seconds
REM SLIC superpixels took 184.746002 seconds
REM  5.4X

REM SLIC superpixels took 19.632000 seconds
REM SLIC superpixels took 184.873993 seconds
REM 9.4X

REM SLIC superpixels took 29.333000 seconds
REM SLIC superpixels took 184.707001 seconds
REM 6.2X

REM AVG: 7X