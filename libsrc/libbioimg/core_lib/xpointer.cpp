/*******************************************************************************

Pointers guarded by reference counting
Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

History:
  2008-01-01 17:02 - First creation

ver: 1

*******************************************************************************/

#include <xpointer.h>

template<typename T>
std::map< T, unsigned int > bim::UniqueReferences<T>::refs;



