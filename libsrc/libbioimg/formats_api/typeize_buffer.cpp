/*******************************************************************************

  Buffer convertions unit: coverts pixels into typed pixels

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2007-07-06 17:02 - First creation
      
  ver: 1
        
*******************************************************************************/

#include <string>
#include <limits>
#include <vector>
#include <cmath>

#include "xtypes.h"

//------------------------------------------------------------------------------------
// 1 Bit
//------------------------------------------------------------------------------------

void cnv_buffer_1to8bit( unsigned char *b8, unsigned char *b1, unsigned int width_pixels ) {

  int rest = width_pixels%8;
  unsigned int w = (unsigned int) floor( width_pixels/8.0 );

  unsigned int x8 = 0;
  unsigned int x1 = 0;

  // if little endian
  //if (!bim::bigendian)
    for (unsigned int x=0; x<w; ++x) {
      b8[x8+0] = b1[x1] >> 7;
      b8[x8+1] = (b1[x1] & 0x40) >> 6;
      b8[x8+2] = (b1[x1] & 0x20) >> 5;
      b8[x8+3] = (b1[x1] & 0x10) >> 4;
      b8[x8+4] = (b1[x1] & 0x08) >> 3;
      b8[x8+5] = (b1[x1] & 0x04) >> 2;
      b8[x8+6] = (b1[x1] & 0x02) >> 1;
      b8[x8+7] = b1[x1] & 0x01;
      x1 += 1;
      x8 += 8;
    } // for x
  /*else
    for (unsigned int x=0; x<w; ++x) {
      b8[x8+7] = b1[x1] >> 7;
      b8[x8+6] = (b1[x1] & 0x40) >> 6;
      b8[x8+5] = (b1[x1] & 0x20) >> 5;
      b8[x8+4] = (b1[x1] & 0x10) >> 4;
      b8[x8+3] = (b1[x1] & 0x08) >> 3;
      b8[x8+2] = (b1[x1] & 0x04) >> 2;
      b8[x8+1] = (b1[x1] & 0x02) >> 1;
      b8[x8+0] = b1[x1] & 0x01;
      x1 += 1;
      x8 += 8;
    } // for x
*/
  // do the last pixel if the size is not even
  //if (!bim::bigendian) {
      if (rest>0) b8[x8+0] = b1[x1] >> 7;
      if (rest>1) b8[x8+1] = (b1[x1] & 0x40) >> 6;
      if (rest>2) b8[x8+2] = (b1[x1] & 0x20) >> 5;
      if (rest>3) b8[x8+3] = (b1[x1] & 0x10) >> 4;
      if (rest>4) b8[x8+4] = (b1[x1] & 0x08) >> 3;
      if (rest>5) b8[x8+5] = (b1[x1] & 0x04) >> 2;
      if (rest>6) b8[x8+6] = (b1[x1] & 0x02) >> 1;
      if (rest>7) b8[x8+7] = b1[x1] & 0x01;
  /*} else {
      if (rest>7) b8[x8+7] = b1[x1] >> 7;
      if (rest>6) b8[x8+6] = (b1[x1] & 0x40) >> 6;
      if (rest>5) b8[x8+5] = (b1[x1] & 0x20) >> 5;
      if (rest>4) b8[x8+4] = (b1[x1] & 0x10) >> 4;
      if (rest>3) b8[x8+3] = (b1[x1] & 0x08) >> 3;
      if (rest>2) b8[x8+2] = (b1[x1] & 0x04) >> 2;
      if (rest>1) b8[x8+1] = (b1[x1] & 0x02) >> 1;
      if (rest>0) b8[x8+0] = b1[x1] & 0x01;
  }*/
}

//------------------------------------------------------------------------------------
// 4 Bit
//------------------------------------------------------------------------------------

void cnv_buffer_4to8bit( unsigned char *b8, unsigned char *b4, unsigned int width_pixels ) {
  
  bool even = ( width_pixels%2 == 0 );
  unsigned int w = (unsigned int) floor( width_pixels/2.0 );

  unsigned int x8 = 0;
  unsigned int x4 = 0;

  // if little endian
  //if (!bim::bigendian)
    for (unsigned int x=0; x<w; ++x) {
      b8[x8+0] = b4[x4] >> 4;
      b8[x8+1] = b4[x4] & 0x0F;
      x4 += 1;
      x8 += 2;
    } // for x
  /*else
    for (unsigned int x=0; x<w; ++x) {
      b8[x8+1] = b4[x4] >> 4;
      b8[x8+0] = b4[x4] & 0x0F;
      x4 += 1;
      x8 += 2;
    } // for x*/

  // do the last pixel if the size is not even
  if (!even)
    //if (!bim::bigendian) {
      b8[x8] = b4[x4] >> 4;
    //} else {
    //  b8[x8] = b4[x4] & 0x0F;
    //}
}

//------------------------------------------------------------------------------------
// 12 Bit
//------------------------------------------------------------------------------------

void cnv_buffer_12to16bit( unsigned char *b16, unsigned char *b12, unsigned int width_pixels ) {
  
  bool even = ( width_pixels%2 == 0 );
  unsigned int w = width_pixels;
  if (!even) w-=1;

  unsigned int x16 = 0;
  unsigned int x12 = 0;

  // if little endian
  if (!bim::bigendian)
    for (unsigned int x=0; x<w; x+=2) {
      b16[x16+1] = b12[x12+0];
      b16[x16+0] = b12[x12+1] >> 4;
      b16[x16+3] = (b12[x12+1] << 4) | (b12[x12+2] >> 4);
      b16[x16+2] = b12[x12+2] & 0x0F;
      x12 += 3;
      x16 += 4;
    } // for x
  else
    for (unsigned int x=0; x<w; x+=2) {
      b16[x16+0] = b12[x12+0];
      b16[x16+1] = b12[x12+1] >> 4;
      b16[x16+2] = (b12[x12+1] << 4) | (b12[x12+2] >> 4);
      b16[x16+3] = b12[x12+2] & 0x0F;
      x12 += 3;
      x16 += 4;
    } // for x

  // do the last pixel if the size is not even
  if (!even)
    if (!bim::bigendian) {
      b16[x16+1] = b12[x12+0];
      b16[x16+0] = b12[x12+1] >> 4;
    } else {
      b16[x16+0] = b12[x12+0];
      b16[x16+1] = b12[x12+1] >> 4;
    }
}

