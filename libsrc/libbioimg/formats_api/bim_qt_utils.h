/*******************************************************************************

  Defines Image Format - Qt4 Utilities
  rely on: DimFiSDK version: 1
  
  Programmer: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    09/13/2005 19:50 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifndef BIM_QT_UTL_H
#define BIM_QT_UTL_H

#include <QImage>
#include <QPixmap>

#include "bim_img_format_interface.h"

QImage  qImagefromDimImage  (const ImageBitmap &img );
QPixmap qPixmapfromDimImage (const ImageBitmap &img );


#endif //BIM_QT_UTL_H


