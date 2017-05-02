/** @file slic.c
** @brief SLIC superpixels - Definition
** @author Andrea Vedaldi
** @author Dmitry Fedorov - templated and parallel version 
**/

/*
Copyright (C) 2007-12 Andrea Vedaldi and Brian Fulkerson.
Copyright (C) 2013 Dmitry Fedorov, Center for Bio-Image informatics
All rights reserved.

This file is part of the VLFeat library and is made available under
the terms of the BSD license (see the COPYING file).
*/

/**
<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->
@page slic Simple Linear Iterative Clustering (SLIC)
@author Andrea Vedaldi
<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->

@ref slic.h implements the Simple Linear Iterative Clustering (SLIC)
algorithm, an image segmentation method described in @cite{achanta10slic}.

- @ref slic-overview
- @ref slic-usage
- @ref slic-tech

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->
@section slic-overview Overview
<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->

SLIC @cite{achanta10slic} is a simple and efficient method to decompose
an image in visually homogeneous regions. It is based on a spatially
localized version of k-means clustering. Similar to mean shift or
quick shift (@ref quickshift.h), each pixel is associated to a feature
vector

@f[
\Psi(x,y) =
\left[
\begin{array}{c}
\lambda x \\
\lambda y \\
I(x,y)
\end{array}
\right]
@f]

and then k-means clustering is run on those. As discussed below, the
coefficient @f$ \lambda @f$ balances the spatial and appearance
components of the feature vectors, imposing a degree of spatial
regularization to the extracted regions.

SLIC takes two parameters: the nominal size of the regions
(superpixels) @c regionSize and the strength of the spatial
regularization @c regularizer. The image is first divided into a grid
with step @c regionSize. The center of each grid tile is then used to
initialize a corresponding k-means (up to a small shift to avoid
image edges). Finally, the k-means centers and clusters are refined by
using the Lloyd algorithm, yielding segmenting the image. As a
further restriction and simplification, during the k-means iterations
each pixel can be assigned to only the <em>2 x 2</em> centers
corresponding to grid tiles adjacent to the pixel.

The parameter @c regularizer sets the trade-off between clustering
appearance and spatial regularization. This is obtained by setting

@f[
\lambda = \frac{\mathtt{regularizer}}{\mathtt{regionSize}}
@f]

in the definition of the feature @f$ \psi(x,y) @f$.

After the k-means step, SLIC optionally
removes any segment whose area is smaller than a threshld @c minRegionSize
by merging them into larger ones.

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->
@section slic-usage Usage from the C library
<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->

To compute the SLIC superpixels of an image use the function
::vl_slic_segment.

<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->
@section slic-tech Technical details
<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  -->

SLIC starts by dividing the image domain into a regular grid with @f$
M \times N @f$ tiles, where

@f[
M = \lceil \frac{\mathtt{imageWidth}}{\mathtt{regionSize}} \rceil,
\quad
N = \lceil \frac{\mathtt{imageHeight}}{\mathtt{regionSize}} \rceil.
@f]

A region (superpixel or k-means cluster) is initialized from each grid
center

@f[
x_i = \operatorname{round} i \frac{\mathtt{imageWidth}}{\mathtt{regionSize}}
\quad
y_j = \operatorname{round} j \frac{\mathtt{imageWidth}}{\mathtt{regionSize}}.
@f]

In order to avoid placing these centers on top of image
discontinuities, the centers are then moved in a 3 x 3
neighbourohood to minimize the edge strength

@f[
\operatorname{edge}(x,y) =
\| I(x+1,y) - I(x-1,y) \|_2^2 +
\| I(x,y+1) - I(x,y-1) \|_2^2.
@f]

Then the regions are obtained by running k-means clustering, started
from the centers

@f[
C = \{ \Psi(x_i,y_j), i=0,1,\dots,M-1\ j=0,1,\dots,N-1 \}
@f]

thus obtained. K-means uses the standard LLoyd algorithm alternating
assigning pixels to the clostest centers a re-estiamting the centers
as the average of the corresponding feature vectors of the pixel
assigned to them. The only difference compared to standard k-means is
that each pixel can be assigned only to the center originated from the
neighbour tiles. This guarantees that there are exactly four
pixel-to-center comparisons at each round of minimization, which
threfore cost @f$ O(n) @f$, where @f$ n @f$ is the number of
superpixels.

After k-means has converged, SLIC eliminates any connected region whose
area is less than @c minRegionSize pixels. This is done by greedily
merging regions to neighbour ones: the pixels @f$ p @f$ are scanned in
lexicographical order and the corresponding connected components
are visited. If a region has already been visited, it is skipped; if not,
its area is computed and if this is less than  @c minRegionSize its label
is changed to the one of a neighbour
region at @f$ p @f$ that has already been vistied (there is always one
except for the very first pixel).

*/

#include <cmath>
#include <cstring>

#include <limits>
#include <algorithm>



/** @brief SLIC superpixel segmentation
** @param segmentation segmentation.
** @param image image to segment.
** @param width image width.
** @param height image height.
** @param numChannels number of image channels (depth).
** @param regionSize nominal size of the regions.
** @param regularization trade-off between appearance and spatial terms.
** @param minRegionSize minimum size of a segment.
**
** The function computes the SLIC superpixels of the specified image @a image.
** @a image is a pointer to an @c width by @c height by @c by numChannles array of @c float.
** @a segmentation is a pointer to a @c width by @c height array of @c vl_uint32.
** @a segmentation contain the labels of each image pixels, from 0 to
** the number of regions minus one.
**
** @sa @ref slic-overview, @ref slic-tech
**/

//#define atimage(x,y,k) image[(x)+(y)*width+(k)*width*height]
#define atimage(x,y,k) ((T*)image->bits((const unsigned int)k))[(x)+(y)*width]
#define atEdgeMap(x,y) edgeMap[(x)+(y)*width]

template <typename T, typename Tw>
void slic_segment (bim::uint32 *segmentation,
    const bim::Image *image, //const T *image,
    bim::uint64 width,
    bim::uint64 height,
    bim::uint64 numChannels,
    bim::uint64 regionSize,
    float regularization,
    bim::uint64 minRegionSize)
{
    bim::uint64 numRegionsX = (bim::uint64) ceil((double) width / regionSize);
    bim::uint64 numRegionsY = (bim::uint64) ceil((double) height / regionSize);
    bim::uint64 numRegions = numRegionsX * numRegionsY;
    bim::uint64 numPixels = width * height;
    bim::uint64 maxNumIterations = 100;

    if (!segmentation) return;
    if (!image) return;
    if (width<1) return;
    if (height<1) return;
    if (numChannels<1) return;
    if (regionSize<1) return;
    if (regularization<0) return;

    Tw * edgeMap = (Tw *) calloc(numPixels, sizeof(Tw)) ;
    Tw * centers = (Tw *) malloc(sizeof(Tw) * (2 + numChannels) * numRegions);
    bim::uint32 * masses = (bim::uint32 *) malloc(sizeof(bim::uint32) * numPixels);

    // compute edge map (gradient strength)
    for (bim::int64 k=0; k<(bim::int64)numChannels; ++k) {
        #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (height>BIM_OMP_FOR2)
        for (bim::int64 y=1; y<(bim::int64)height-1; ++y) {
            for (bim::int64 x=1; x<(bim::int64)width-1; ++x) {
                Tw a = (Tw) atimage(x-1,y,k);
                Tw b = (Tw) atimage(x+1,y,k);
                Tw c = (Tw) atimage(x,y+1,k);
                Tw d = (Tw) atimage(x,y-1,k);
                atEdgeMap(x,y) += (a - b)  * (a - b) + (c - d) * (c - d) ;
            }
        }
    }

    // initialize K-means centers
    #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (numRegionsY>BIM_OMP_FOR2)
    for (bim::int64 v = 0 ; v < (bim::int64)numRegionsY ; ++v) {
        for (bim::int64 u = 0 ; u < (bim::int64)numRegionsX ; ++u) {
            Tw minEdgeValue = std::numeric_limits<Tw>::infinity();

            bim::int64 x = bim::round<bim::int64>(regionSize * (u + 0.5)) ;
            bim::int64 y = bim::round<bim::int64>(regionSize * (v + 0.5)) ;
            x = bim::max<bim::int64>(bim::min<bim::int64>(x, width-1), 0);
            y = bim::max<bim::int64>(bim::min<bim::int64>(y, height-1), 0);

            bim::int64 centerx, centery;
            // search in a 3x3 neighbourhood the smallest edge response
            for (bim::int64 yp = bim::max<bim::int64>(0, y-1) ; yp <= bim::min<bim::int64>(height-1, y+1) ; ++ yp) {
                for (bim::int64 xp = bim::max<bim::int64>(0, x-1) ; xp <= bim::min<bim::int64>(width-1, x+1) ; ++ xp) {
                    Tw thisEdgeValue = atEdgeMap(xp,yp) ;
                    if (thisEdgeValue < minEdgeValue) {
                        minEdgeValue = thisEdgeValue;
                        centerx = xp;
                        centery = yp;
                    }
                }
            }

            // initialize the new center at this location
            bim::int64 i = (v*numRegionsX + u) * (2+numChannels);
            centers[i++] = (Tw) centerx;
            centers[i++] = (Tw) centery;
            for (bim::int64 k=0; k<(bim::int64)numChannels; ++k) {
                centers[i++] = (Tw) atimage(centerx,centery,k) ;
            }
        }
    }
    free(edgeMap);

    // run k-means iterations
    Tw previousEnergy = std::numeric_limits<Tw>::infinity();
    Tw startingEnergy;

    for (bim::int64 iter=0; iter<(bim::int64)maxNumIterations; ++iter) {
        Tw factor = regularization / (regionSize * regionSize);
        Tw energy = 0;

        // assign pixels to centers
        for (bim::int64 y=0; y<(bim::int64)height; ++y) {
            std::vector<Tw> distances(width, 0);
            #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (width>BIM_OMP_FOR2)
            for (bim::int64 x=0; x<(bim::int64)width; ++x) {
                bim::int64 u = (bim::int64) floor((double)x / regionSize - 0.5) ;
                bim::int64 v = (bim::int64) floor((double)y / regionSize - 0.5) ;
                Tw minDistance = std::numeric_limits<Tw>::infinity();

                for (bim::int64 vp = bim::max<bim::int64>(0, v) ; vp <= bim::min<bim::int64>(numRegionsY-1, v+1) ; ++vp) {
                    for (bim::int64 up = bim::max<bim::int64>(0, u) ; up <= bim::min<bim::int64>(numRegionsX-1, u+1) ; ++up) {
                        bim::int64 region = up  + vp * numRegionsX;
                        Tw centerx = centers[(2 + numChannels) * region + 0];
                        Tw centery = centers[(2 + numChannels) * region + 1];
                        Tw spatial = (x - centerx) * (x - centerx) + (y - centery) * (y - centery);
                        Tw appearance = 0;
                        for (bim::int64 k = 0 ; k < (bim::int64)numChannels ; ++k) {
                            Tw centerz = centers[(2 + numChannels) * region + k + 2]  ;
                            Tw z = (Tw) atimage(x,y,k) ;
                            appearance += (z - centerz) * (z - centerz);
                        }
                        Tw distance = appearance + factor * spatial;
                        if (minDistance > distance) { // recompare since it could have been changed by another thread
                            minDistance = distance ;
                            segmentation[x + y * width] = (bim::uint32)region;
                        }

                    }
                }

                //energy += minDistance; // dima: should be critical block, but that is slow, use distances array instead
                distances[x] = minDistance;
            } // x

            for (bim::int64 x=0; x<(bim::int64)width; ++x)
                energy += distances[x];

        } // y

        // check energy termination conditions
        if (iter == 0) {
            startingEnergy = energy ;
        } else {
            if ((previousEnergy - energy) < 1e-5 * (startingEnergy - energy)) {
                break ;
            }
        }
        previousEnergy = energy ;

        // recompute centers
        memset(masses, 0, sizeof(bim::uint32) * width * height) ;
        memset(centers, 0, sizeof(Tw) * (2 + numChannels) * numRegions) ;

        #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (height>BIM_OMP_FOR2)        
        for (bim::int64 y = 0 ; y < (bim::int64)height ; ++y) {
            for (bim::int64 x = 0 ; x < (bim::int64)width ; ++x) {
                bim::int64 pixel = x + y * width ;
                bim::int64 region = segmentation[pixel] ;
                masses[region] ++ ;
                centers[region * (2 + numChannels) + 0] += x ;
                centers[region * (2 + numChannels) + 1] += y ;
                for (bim::int64 k = 0 ; k < (bim::int64)numChannels ; ++k) {
                    centers[region * (2 + numChannels) + k + 2] += (Tw) atimage(x,y,k) ;
                }
            }
        }

        #pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (numRegions>BIM_OMP_FOR2)
        for (bim::int64 region = 0 ; region < (bim::int64)numRegions ; ++region) {
            Tw mass = bim::max<Tw>((Tw)masses[region], 1e-8f) ;
            for (bim::int64 i = (2 + numChannels) * region ;
                i < (bim::int64)(2 + numChannels) * (region + 1) ;
                ++i) {
                    centers[i] /= mass ;
            }
        }
    }

    free(masses) ;
    free(centers) ;

    // elimiate small regions
    {
        bim::uint32 * cleaned = (bim::uint32 *) calloc(numPixels, sizeof(bim::uint32)) ;
        bim::uint64 * segment = (bim::uint64 *) malloc(sizeof(bim::uint64) * numPixels) ;
        bim::int64 dx [] = {+1, -1,  0,  0} ;
        bim::int64 dy [] = { 0,  0, +1, -1} ;

        for (bim::int64 pixel = 0 ; pixel < (bim::int64)numPixels ; ++pixel) {
            if (cleaned[pixel]) continue;
            bim::uint32 label = segmentation[pixel];
            bim::uint64 numExpanded = 0;
            bim::uint64 segmentSize = 0;
            segment[segmentSize++] = pixel;

            // find cleanedLabel as the label of an already cleaned
            // region neihbour of this pixel
            bim::uint32 cleanedLabel = label + 1 ;
            cleaned[pixel] = label + 1 ;
            bim::int64 x = pixel % width ;
            bim::int64 y = pixel / width ;
            for (bim::int64 direction = 0 ; direction < 4 ; ++direction) {
                bim::int64 xp = x + dx[direction] ;
                bim::int64 yp = y + dy[direction] ;
                bim::int64 neighbor = xp + yp * width ;
                if (0 <= xp && xp < (bim::int64)width &&
                    0 <= yp && yp < (bim::int64)height &&
                    cleaned[neighbor]) {
                        cleanedLabel = cleaned[neighbor] ;
                }
            }

            // expand the segment
            while (numExpanded < segmentSize) {
                bim::int64 open = segment[numExpanded++] ;
                x = open % width ;
                y = open / width ;
                for (bim::int64 direction = 0 ; direction < 4 ; ++direction) {
                    bim::int64 xp = x + dx[direction] ;
                    bim::int64 yp = y + dy[direction] ;
                    bim::int64 neighbor = xp + yp * width ;
                    if (0 <= xp && xp < (bim::int64)width &&
                        0 <= yp && yp < (bim::int64)height &&
                        cleaned[neighbor] == 0 &&
                        segmentation[neighbor] == label) {
                            cleaned[neighbor] = label + 1 ;
                            segment[segmentSize++] = neighbor ;
                    }
                }
            }

            // change label to cleanedLabel if the semgent is too small
            if (segmentSize < minRegionSize) {
                while (segmentSize > 0) {
                    cleaned[segment[--segmentSize]] = cleanedLabel ;
                }
            }
        }
        // restore base 0 indexing of the regions
        for (bim::int64 pixel = 0 ; pixel < (bim::int64)numPixels ; ++pixel) cleaned[pixel] -- ;

        memcpy(segmentation, cleaned, numPixels * sizeof(bim::uint32)) ;
        free(cleaned) ;
        free(segment) ;
    }
}
