/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT: 
  xy1, xy2 - 1D arrays of TPoint (start with 1)
  len1 - is the length of xy1

OUTPUT:
  diff - pointer to double
  im1, im2 - 1D arrays of int (start with 1)
  xym1, xym2 - 1D arrays of TPoint (start with 1)
  len2 - pointer to int, length of im1, im2, xym1, xym2

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   08/01/2001 18:40:30 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 2
*****************************************************************************/

#ifndef REG_HARMONIC_DIFFERENCE
#define REG_HARMONIC_DIFFERENCE

template< typename Twork, typename Tpoint >
void harmonicdiff( const std::deque< reg::Point<Tpoint> > *xy1,
                   const std::deque< reg::Point<Tpoint> > *xy2, 
                   double *diff, 
                   std::vector<int> *im1, 
                   std::vector<int> *im2,
                   std::deque< reg::Point<Tpoint> > *xym1,
                   std::deque< reg::Point<Tpoint> > *xym2 )
{
  *diff = 0;
  xym1->clear();
  xym2->clear();
  im1->clear();
  im2->clear();

  for (int i=0; i<xy1->size(); ++i) {
    double d = fabs((*xy1)[i].x-(*xy2)[i].x) + fabs((*xy1)[i].y-(*xy2)[i].y);
    if (d<5) d = 1; else d = 0;
    *diff = *diff + d;

    if (d >= 0.5) {
      // hey we got a pretty good match - so save the match!
      im1->push_back(i);
      im2->push_back(i);
      xym1->push_back( (*xy1)[i] );
      xym2->push_back( (*xy2)[i] );
    }
  }
}

#endif // REG_HARMONIC_DIFFERENCE
