/*******************************************************************************

  Implementation of the Image Class for ITK
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    2011-05-11 08:32:12 - First creation
      
  ver: 1
        
*******************************************************************************/

#ifdef BIM_USE_ITK
#pragma message("Image: ITK support methods")

#include "bim_image.h"

#include <algorithm>
#include <limits>

// ITK includes
#include "itkImage.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
//#include "itkImportImageFilter.h"

template <typename PixelType>
itk::Image< PixelType, 2 >::Pointer Image::toItkImage( const unsigned int &channel ) const {

  int sample = std::min<int>( this->samples()-1, channel); 
  int bitdepth = sizeof(PixelType);
  DataFormat pf = FMT_UNSIGNED;
  if (!std::numeric_limits<PixelType>::is_integer) 
    pf = FMT_FLOAT;
  else
  if (std::numeric_limits<PixelType>::is_signed) 
    pf = FMT_SIGNED;
  
  Image img;
  if (this->depth()!=bitdepth || this->pixelType()!=pf) {
    img = this->ensureTypedDepth();
    img = img.convertToDepth( bitdepth, Lut::ltLinearDataRange, pf );
  } else
    img = *this;

  //------------------------------------------------------------------
  // Create Itk Image and copy data
  //------------------------------------------------------------------
  ImageType::Pointer image = ImageType::New();
  ImageType::SizeType size;
  size[0] = this->width();
  size[1] = this->height();

  ImageType::IndexType start;
  start[0] = 0;
  start[1] = 0;  

  ImageType::RegionType region;
  region.SetSize( size );
  region.SetIndex( start );
  image->SetRegions( region );

  image->Allocate();

  double spacing[2];
  spacing[0] = 1.0;
  spacing[1] = 1.0;
  image->SetSpacing( spacing );

  double origin[2];
  origin[0] = 0.0;
  origin[1] = 0.0;
  image->SetOrigin( origin );

  typedef itk::ImageRegionIterator< ImageType >  IteratorType;
  IteratorType it( image, region );
  it.GoToBegin();

  // copy data
  PixelType *data = (PixelType *) img.sampleBits(sample);
  while( ! it.IsAtEnd() ) {
    it.Set( *data );
    ++it;
    ++data;
  }
  return image;
}

/*
typedef itk::Image< char, 2 >  ImageType;
ImageType::Pointer image_transfer( 
                                  int nx, int ny, const char * buffer, 
                                  float dx, float dy ,float ox, float oy )
{
  typedef itk::ImportImageFilter< char, 2 >  ImportFilterType;
  ImportFilterType::Pointer importer = ImportFilterType::New();
  ImageType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  ImageType::IndexType start;
  start[0] = 0;
  start[1] = 0;

  ImageType::RegionType region;
  region.SetSize( size );
  region.SetIndex( start );
  importer->SetRegions( region );
  double spacing[2];
  spacing[0] = dx;
  spacing[1] = dy;
  importer->SetSpacing( spacing );
  double origin[2];
  origin[0] = ox;
  origin[1] = oy;
  importer->SetOrigin( origin );


  const bool importFilterWillDeleteTheInputBuffer = false;
  typedef     ImageType::PixelType PixelType;
  PixelType * pixelData = static_cast< PixelType * >( buffer );
  const unsigned int totalNumberOfPixels = nx * ny;
  import->SetImportPointer( 
    pixelData, 
    totalNumberOfPixels, 
    importFilterWillDeleteTheInputBuffer );
  importer->Update();
  return importer->GetOutput();
}
*/

template <typename PixelType>
void Image::fromItkImage( const itk::Image< PixelType, 2 > *image ) {
  
  ImageType::RegionType region;
  region = image->GetBufferedRegion();
  ImageType::SizeType size = region.GetSize();
  ImageType::IndexType start = region.GetIndex();
  unsigned int nx = size[0];
  unsigned int ny = size[1];

  // allocate this image
  if (this->alloc( nx, ny, 1, sizeof(PixelType))<=0) return;

  // copy data
  typedef itk::ImageRegionConstIterator< ImageType >  IteratorType;
  IteratorType it( image, region );
  it.GoToBegin();
  const PixelType *data = (PixelType *) this->sampleBits(0);
  while( !it.IsAtEnd() ) {
    *data = it.Get();
    ++it;
    ++data;
  }
}

#endif //BIM_USE_ITK
