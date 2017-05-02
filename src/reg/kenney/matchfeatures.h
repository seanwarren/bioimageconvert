/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  Matw, MatW - TMatrix: 2-D arrays of REAL
  xyA, XYA   - pointers to TPointA

OUTPUT:
  xxyyA, XXYYA - pointers to TPointA

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   07/24/2001 18:40:30 - First creation
   08/10/2001 15:06:32 - use of TPointA
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_MATCH_FEATURES
#define REG_MATCH_FEATURES

template< typename Timg, typename Tpoint >
int matchfeatures(const reg::Matrix<Timg> *Matw, const reg::Matrix<Timg> *MatW, 
                  const std::deque< reg::Point<Tpoint> > *candidates1, 
                  const std::deque< reg::Point<Tpoint> > *candidates2, 
                  int numpoints, 
                  std::deque< reg::Point<Tpoint> > *points1,
                  std::deque< reg::Point<Tpoint> > *points2 )
// now for each point in (x,y) find the closest point in (X,Y)
// as measured by subtracting the windows and picking
// the (X,Y) pair with the least difference
{  
  int rw = Matw->rows-2; // -2 ignore rotation parameters
  int cw = Matw->cols;
  Timg **w =  (Timg **)Matw->data;
  int rW = MatW->rows-2; // -2 ignore rotation parameters
  int cW = MatW->cols;
  Timg **W =  (Timg **)MatW->data;


  Timg **dk = m_init<Timg>(cw, cw);
  std::vector<int> kKmin(cw+1);
  std::vector<int> Kkmin(cw+1);

  #pragma omp parallel for default(shared)
  for (int k=1; k<=cw; k++)
  for (int j=1; j<=cw; j++) {
    dk[k][j] = 0;
    for (int i=1; i<=rw; i++)
      dk[k][j] += (Timg) fabs( w[i][k]-W[i][j] );
  }

  #pragma omp parallel for default(shared)
  for (int k=1; k<=cW; k++) {
    kKmin[k]=1;
    Timg minT = dk[k][1];
    for (int j=2; j<=cw; j++) if (minT>dk[k][j]) { minT=dk[k][j]; kKmin[k]=j;}
    
    Kkmin[k]=1;
    minT = dk[1][k];
    for (int i=2; i<=cw; i++) if (minT>dk[i][k]) { minT=dk[i][k]; Kkmin[k]=i;}
  }

  numpoints = (int) std::min<size_t>( numpoints, std::min<size_t>(candidates1->size(), candidates2->size()) );

  Timg **dkt = m_init<Timg>(cw, cw);
  m_transpose(dk, cw, cw, dkt);
  m_free(dk); dk = dkt;
  std::vector<Timg> mdk(cw+1);
  m_min(dk, cw, cw, &mdk[0], 1);

  std::vector<int> iii(cw+1);
  a_sort_index(&mdk[0], 1, cw, &iii[0]);

  int numnum=0;
  for (int i=1; i<=cw; i++) {
    int k=iii[i];
    int K=kKmin[k];
    if (Kkmin[K]==k) {
      numnum=numnum+1;
      points1->push_back( (*candidates1)[k-1] );
      points2->push_back( (*candidates2)[K-1] );
      if (numnum==numpoints) break;
    }
  }

  // check to make sure we got enough points
  // if not the reduce the match requirements
  if (numnum<numpoints/2) {
    numnum=0;
    for (int i=1; i<=cw; i++) {
     int k=iii[i];
     int K=kKmin[k];
     numnum=numnum+1;
     points1->push_back( (*candidates1)[k-1] );
     points2->push_back( (*candidates2)[K-1] );
     if (numnum==numpoints) break;
    }
  }

  m_free(dk);
  return 0;
}

#endif // REG_MATCH_FEATURES
