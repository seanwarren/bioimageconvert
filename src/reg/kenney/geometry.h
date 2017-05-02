/*****************************************************************************
  Original Matlab code by Charles Kenney 
  Converted by Dima V. Fedorov, 2001

INPUT:
  mg, MG - pointers to TMatrix
  xyA, XYA - pointers to TPointA

OUTPUT:
  r, c, s, dx, dy - pointers to double
  xysA, XYsA - pointers to TPointA, do not init them!!!

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

 History:
   07/25/2001 18:40:30 - First creation
   08/10/2001 15:06:32 - use of TPointA
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 3
*****************************************************************************/

#ifndef REG_GEOMETRY_RANSAC
#define REG_GEOMETRY_RANSAC

template< typename Timg, typename Tpoint, typename Tw >
void geometry( const reg::Matrix<Timg> *mg, const reg::Matrix<Timg> *MG, 
               const std::deque< reg::Point<Tpoint> > *matched1,
               const std::deque< reg::Point<Tpoint> > *matched2,
               double *r, double *c, double *s, double *dx, double *dy,
               std::deque< reg::Point<Tpoint> > *points1,
               std::deque< reg::Point<Tpoint> > *points2 )
{
  // use the geometry of the tiepoints to find a good match

  int numbo=0;
  double ranger=0.2;   // limit on deviation of the r factor from 1
  int diffmax=0;
  Timg mm[3][3];
  Timg V[3][2];

  Timg **v = m_init<Timg>(2, 1);
  Timg **vN = m_init<Timg>(4, 1);

  int numdist = bim::round<int>(std::min<double>(mg->rows, mg->cols)/4.0);

  // make sure we'll have empty lists if RANSAC fails
  points1->clear();
  points2->clear();
  
  getRSTparameters<Tpoint, Tw>(matched1, matched2, r, c, s, dx, dy);

  double fitmin, mv, sv, mV, sV, overlap;
  evalfit<Timg, Tw>(mg, MG, *r, *c, *s, *dx, *dy, &fitmin, &mv, &sv, &mV, &sV, &overlap);
  fitmin += REG_EPS; // in case we are registering something matching PERFECTLY, we have to ensure output point lists

  int rows = mg->rows;
  int cols = mg->cols;

  for (int i1=0; i1<matched1->size(); ++i1)
  for (int i2=0; i2<matched1->size(); ++i2) {
    if ( i1 != i2 ) {
      // get a pair of points from the x list
      reg::Point<Tpoint> xy1 = (*matched1)[i1]; 
      reg::Point<Tpoint> xy2 = (*matched1)[i2]; 
      
      //d=[x2-x1,y2-y1];
      Tw d[3];  
      d[1] = xy2.y - xy1.y;
      d[2] = xy2.x - xy1.x;

      //nd=norm(d);
      double nd = a_norm(d, 1, 2);

      if (nd>numdist) {
        //dd = d/nd;
        Tw dd[3];  
        dd[1] = (Tw)(d[1]/nd);
        dd[2] = (Tw)(d[2]/nd);

        // find the best match for (x(2)-x(1),y(2)-y(1))
        int I1=i1;
        int I2=i2;
        // get a pair of points from the X list
        reg::Point<Tpoint> XY1 = (*matched2)[I1]; 
        reg::Point<Tpoint> XY2 = (*matched2)[I2]; 

        //D=[X2-X1,Y2-Y1];
        Tw D[3];  
        D[1] = XY2.y - XY1.y;
        D[2] = XY2.x - XY1.x;

        //nD=norm(D);
        double nD = a_norm(D, 1, 2);

        if (nD>0) {
          //DD=D/nD;
          double DD[3];
          DD[1] = ((double) D[1])/nD;
          DD[2] = ((double) D[2])/nD;

          Tw rr = (Timg) (nD/nd);

          if (fabs(rr-1)<ranger) {
            // Get the cosine of the angle of rotation
            //cc=sum(sum(DD.*dd));
            Tw cc = (Timg) (DD[1]*dd[1]+DD[2]*dd[2]);
            Tw ss=0;

            // find ss
            if ( fabs(xy2.x-xy1.x) > fabs(xy2.y-xy1.y) )
              ss = (((Tw) (XY2.y-XY1.y))-rr*cc*((Tw) (xy2.y-xy1.y)))/(rr*((Tw) (xy2.x-xy1.x)));
            else
              ss =-(((Tw) (XY2.x-XY1.x))-rr*cc*((Tw) (xy2.x-xy1.x)))/(rr*((Tw) (xy2.y-xy1.y)));

            // Hey we need to find the shift!
            //mm=rr*[cc,ss
            //    -ss,cc];
            mm[1][1] = rr*cc;
            mm[1][2] = rr*ss;
            mm[2][1] = -rr*ss;
            mm[2][2] = rr*cc;
            
            //v= [x1
            //    y1];
            v[1][1] = (Timg) xy1.y;
            v[2][1] = (Timg) xy1.x;

            //V= [X1
            //    Y1];
            V[1][1] = (Timg) XY1.y;
            V[2][1] = (Timg) XY1.x;

            //dxdy=V-mm*v;
            //ddxx=dxdy(1); ddyy=dxdy(2);
            double ddxx = V[1][1] - (mm[1][1]*v[1][1]+mm[1][2]*v[2][1]);
            double ddyy = V[2][1] - (mm[2][1]*v[1][1]+mm[2][2]*v[2][1]);
 
            // do the transformation to x,y
            // and don't forget the scale factor!
            //xx =  rr*cc*x+rr*ss*y+ddxx;
            //yy = -rr*ss*x+rr*cc*y+ddyy;
            std::deque< reg::Point<Tpoint> > xxyy(matched1->size());
            for (int td=0; td<matched1->size(); ++td) {
              xxyy[td].y = (Tpoint)(  rr*cc*(*matched1)[td].y + rr*ss*(*matched1)[td].x + ddxx );
              xxyy[td].x = (Tpoint)( -rr*ss*(*matched1)[td].y + rr*cc*(*matched1)[td].x + ddyy );
            }
     
            // now get the harmonic distance
            //[diff,im1,im2]=harmonicdiff(xxyy, XY);
            double diff;
            std::vector<int> im1;
            std::vector<int> im2;
            std::deque< reg::Point<Tpoint> > xym1;
            std::deque< reg::Point<Tpoint> > xym2;
            harmonicdiff<Tw, Tpoint>(&xxyy, matched2, &diff, &im1, &im2, &xym1, &xym2);

            if (diff>diffmax) {
              diffmax= (int) diff;
              numbo=numbo+1;
 	            // recalculate r c s dx dy using all 
	            // the matched points as listed in im1 and im2
              int lenmatch = (int) xym1.size();
              std::deque< reg::Point<Tpoint> > xyss(xym1.size());
              std::deque< reg::Point<Tpoint> > XYss(xym1.size());

              Timg **mat = m_init<Timg>(lenmatch*2, 4);
              Timg **b   = m_init<Timg>(lenmatch*2, 1); 
              for (int k=1; k<=lenmatch; k++) {
                xyss[k-1] = (*matched1)[im1[k-1]]; 
                XYss[k-1] = (*matched2)[im2[k-1]]; 
                 
                mat[2*k-1][1] = (*matched1)[im1[k-1]].y;
                mat[2*k-1][2] = (*matched1)[im1[k-1]].x;
                mat[2*k-1][3] = 1;
                mat[2*k-1][4] = 0;
                b[2*k-1][1] = (*matched2)[im2[k-1]].y;

                mat[2*k][1] =  (*matched1)[im1[k-1]].x;
                mat[2*k][2] = -(*matched1)[im1[k-1]].y;
                mat[2*k][3] = 0;
                mat[2*k][4] = 1;
                b[2*k][1] = (*matched2)[im2[k-1]].x;
              }

	            //v=pinv(mat)*b;
              Timg **temp = m_init<Timg>(4, 2*lenmatch);
              pinv<Timg>(mat, 2*lenmatch, 4, temp);
              m_mult(temp, b, 4, 2*lenmatch, 2*lenmatch, 1, vN);
              m_free(temp);

              //rr=sqrt(v(1)*v(1)+v(2)*v(2));
              rr  = (Timg) sqrt(vN[1][1] * vN[1][1] + vN[2][1] * vN[2][1]);

              cc   = vN[1][1] / rr;
              ss   = vN[2][1] / rr;
              ddxx = vN[3][1];
              ddyy = vN[4][1];

              //fit=evalfit(g,G,rr,cc,ss,ddxx,ddyy);
              double fit;
              evalfit<Timg, Tw>(mg, MG, rr, cc, ss, ddxx, ddyy, &fit, &mv, &sv, &mV, &sV, &overlap);
 
              if (fit<fitmin) {
                fitmin=fit;
	              *r=rr;                           
	              *c=cc;     
	              *s=ss;       
	              *dx=ddxx;
	              *dy=ddyy;

                points1->assign( xyss.begin(), xyss.end() );
                points2->assign( XYss.begin(), XYss.end() );
              } // fitmin>fit check between g and G for the given parameters r a dx dy
              m_free(b);
              m_free(mat);
            } // diffmax>diff check
          } // check on range of r
        } // check on nD=0
      } // check for nd>numdist
    } // check for i1=i2
  } // i1
  m_free(v);
  m_free(vN);
}

#endif // REG_GEOMETRY_RANSAC
