/*******************************************************************************

  Buffer convertions unit: coverts pixels into typed pixels

  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2007-07-06 17:02 - First creation

  ver: 1

*******************************************************************************/

#ifndef TYPEIZE_BUFFER_H
#define TYPEIZE_BUFFER_H


void cnv_buffer_1to8bit(unsigned char *b8, const unsigned char *b1, const unsigned int width_pixels);

void cnv_buffer_4to8bit(unsigned char *b8, const unsigned char *b4, const unsigned int width_pixels);

void cnv_buffer_12to16bit(unsigned char *b16, const unsigned char *b12, const unsigned int width_pixels);


#endif
