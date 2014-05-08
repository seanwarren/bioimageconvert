/*******************************************************************************

  Implementation of the Image Class for Windows API
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    03/24/2006 12:45 - First creation
      
  ver: 1
        
*******************************************************************************/

#if defined(WIN32) || defined(_WIN32) || defined(WIN64)  || defined(_WIN64) 

#pragma message("Image: WinAPI support methods")

#include "bim_image.h"
#include "bim_img_format_utils.h"

using namespace bim;

HBITMAP Image::toWinHBITMAP() const {
  HBITMAP hbmp = 0;

  return hbmp;
}

#endif //WIN32

