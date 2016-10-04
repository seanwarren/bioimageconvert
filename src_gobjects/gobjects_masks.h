/*******************************************************************************

GObject Masks

Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>
        Center for Bio-image Informatics, UCSB

History:
2008-01-01 17:02 - First creation

ver: 1

*******************************************************************************/

#ifndef DIM_GOBJECTS_MASKS_H
#define DIM_GOBJECTS_MASKS_H

#include <QObject>
#include <QColor>

#include "gobjects.h"

//#define BIM_USE_PROGRESS

class DProgressWidget;

//---------------------------------------------------------------------------------
// rendering functions
//---------------------------------------------------------------------------------

void renderGObjectsToImage( const QString &name, const QString &format, 
                       DGObjects &gobjects, int w, int h, int l,
                       DProgressWidget *progress = 0, 
                       const DGObjectRenderingOptions &opts = DGObjectRenderingOptions(),
                       const QString &args = QString() );

#endif // DIM_GOBJECTS_MASKS_H
