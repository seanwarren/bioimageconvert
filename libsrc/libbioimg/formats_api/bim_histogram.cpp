/*******************************************************************************

  histogram and lutf
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    10/20/2006 20:21 - First creation
    2007-06-26 12:18 - added lut class
    2010-01-22 17:06 - changed interface to support floating point data

  ver: 3
        
*******************************************************************************/

#include "bim_histogram.h"
#include "xtypes.h"
#include "xstring.h"

#include <cmath>
#include <limits>
#include <cstring>

#include <limits>

using namespace bim;

//******************************************************************************
// Histogram
//******************************************************************************

Histogram::Histogram( const unsigned int &bpp, const DataFormat &fmt ) {
  default_size = Histogram::defaultSize;
  init( bpp, fmt ); 
}

Histogram::Histogram( const unsigned int &bpp, void *data, const unsigned int &num_data_points, const DataFormat &fmt, unsigned char *mask ) {
  default_size = Histogram::defaultSize;
  newData( bpp, data, num_data_points, fmt, mask );
}

Histogram::~Histogram() {

}

void Histogram::init( const unsigned int &bpp, const DataFormat &fmt ) {
  d.data_bpp = bpp;
  d.data_fmt = fmt;

  d.shift=0; d.scale=1;
  d.value_min=0; d.value_max=0;
  reversed_min_max = false;
  hist.resize(0);

  if (bpp==0) return;
  else
  if ( bpp<=16 && (d.data_fmt==FMT_UNSIGNED || d.data_fmt==FMT_SIGNED) ) {
    hist.resize( (unsigned int) pow(2.0, fabs((double) bpp)), 0 );
  } else 
  if (bpp>16 || d.data_fmt==FMT_FLOAT) {
    hist.resize( default_size );
  }
  initStats();
}

void Histogram::clear( ) {
  hist = std::vector<Histogram::StorageType>(hist.size(), 0); 
}

void Histogram::newData( const unsigned int &bpp, void *data, const unsigned int &num_data_points, const DataFormat &fmt, unsigned char *mask ) {
  init( bpp, fmt );
  clear( );
  getStats( data, num_data_points, mask );
  addData( data, num_data_points, mask );
}

template <typename T> 
void Histogram::init_stats() {
  d.value_min = std::numeric_limits<T>::min();
  d.value_max = std::numeric_limits<T>::max();
  reversed_min_max = false;
  recompute_shift_scale();
}

void Histogram::initStats() {
  if (d.data_bpp==8  && d.data_fmt==FMT_UNSIGNED) init_stats<uint8>();
  else
  if (d.data_bpp==16 && d.data_fmt==FMT_UNSIGNED) init_stats<uint16>();
  else   
  if (d.data_bpp==32 && d.data_fmt==FMT_UNSIGNED) init_stats<uint32>();
  else
  if (d.data_bpp==8  && d.data_fmt==FMT_SIGNED)   init_stats<int8>();
  else
  if (d.data_bpp==16 && d.data_fmt==FMT_SIGNED)   init_stats<int16>();
  else
  if (d.data_bpp==32 && d.data_fmt==FMT_SIGNED)   init_stats<int32>();
  else
  if (d.data_bpp==32 && d.data_fmt==FMT_FLOAT)    init_stats<float32>();
  else
  if (d.data_bpp==64 && d.data_fmt==FMT_FLOAT)    init_stats<float64>();
  else
  if (d.data_bpp==80 && d.data_fmt==FMT_FLOAT)    init_stats<float80>();
}

inline void Histogram::recompute_shift_scale() {
  d.shift = d.value_min;
  d.scale = ((double) bin_number()-1) / (d.value_max-d.value_min);
}

template <typename T>
void Histogram::update_data_stats( T *data, const unsigned int &num_data_points, unsigned char *mask ) {
  if (!data) return;
  if (num_data_points==0) return;
  if (!reversed_min_max) {
    d.value_min = std::numeric_limits<T>::max();
    d.value_max = std::numeric_limits<T>::min();
    reversed_min_max = true;
  }

  T *p = (T *) data;  
  if (mask == 0) {
    for (unsigned int i=0; i<num_data_points; ++i) {
      if (*p<d.value_min) d.value_min = *p;
      if (*p>d.value_max) d.value_max = *p;
      ++p;
    }
  } else {
    for (unsigned int i=0; i<num_data_points; ++i) {
      if (mask[i]>128) {
        if (*p<d.value_min) d.value_min = *p;
        if (*p>d.value_max) d.value_max = *p;
      }
      ++p;
    }
  }
  
  recompute_shift_scale();
}

template <typename T>
void Histogram::get_data_stats( T *data, const unsigned int &num_data_points, unsigned char *mask ) {
  init_stats<T>();
  d.value_min = std::numeric_limits<T>::max();
  d.value_max = std::numeric_limits<T>::min();
  reversed_min_max = true;
  update_data_stats(data, num_data_points, mask);
}

void Histogram::getStats( void *data, const unsigned int &num_data_points, unsigned char *mask ) {
  if (d.data_fmt==FMT_UNSIGNED) {
    if (d.data_bpp == 32) get_data_stats<uint32>( (uint32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_SIGNED) {
    if (d.data_bpp == 32) get_data_stats<int32>( (int32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_FLOAT) {
    if (d.data_bpp == 32) get_data_stats<float32>( (float32*)data, num_data_points, mask );
    else
    if (d.data_bpp == 64) get_data_stats<float64>( (float64*)data, num_data_points, mask );
    else
    if (d.data_bpp == 80) get_data_stats<float80>( (float80*)data, num_data_points, mask );
  }
}

template <typename T>
inline unsigned int Histogram::bin_from( const T &data_point ) const {
  return bim::trim<unsigned int, double>( (data_point - d.shift) * d.scale, 0, bin_number() );
}

template <typename T>
void Histogram::add_from_data( T *data, const unsigned int &num_data_points, unsigned char *mask ) {
  if (!data) return;
  if (num_data_points==0) return;

  T *p = (T *) data;  
  if (mask==0) {
    for (unsigned int i=0; i<num_data_points; ++i) {
      ++hist[*p];
      ++p;
    }
  } else {
    for (unsigned int i=0; i<num_data_points; ++i) {
      if (mask[i]>128) ++hist[*p];
      ++p;
    }
  }
}

template <typename T>
void Histogram::add_from_data_scale( T *data, const unsigned int &num_data_points, unsigned char *mask ) {
  if (!data) return;
  if (num_data_points==0) return;
  //get_data_stats( data, num_data_points );

  T *p = (T *) data; 
  unsigned int bn = bin_number();
  if (mask==0) {
    for (unsigned int i=0; i<num_data_points; ++i) {
      double v = (((double)*p) - d.shift) * d.scale;
      unsigned int bin = bim::trim<unsigned int, double>( v, 0, bn-1 );
      ++hist[bin];
      ++p;
    }
  } else {
    for (unsigned int i=0; i<num_data_points; ++i) {
      if (mask[i]>128) {
        double v = (((double)*p) - d.shift) * d.scale;
        unsigned int bin = bim::trim<unsigned int, double>( v, 0, bn-1 );
        ++hist[bin];
      }
      ++p;
    }
  }
}

void Histogram::updateStats( void *data, const unsigned int &num_data_points, unsigned char *mask ) {
  if (d.data_fmt==FMT_UNSIGNED) {
    if (d.data_bpp == 32) update_data_stats<uint32>( (uint32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_SIGNED) {
    if (d.data_bpp == 32) update_data_stats<int32>( (int32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_FLOAT) {
    if (d.data_bpp == 32) update_data_stats<float32>( (float32*)data, num_data_points, mask );
    else
    if (d.data_bpp == 64) update_data_stats<float64>( (float64*)data, num_data_points, mask );
    else
    if (d.data_bpp == 80) update_data_stats<float80>( (float80*)data, num_data_points, mask );
  }
}

void Histogram::addData( void *data, const unsigned int &num_data_points, unsigned char *mask ) {

  if (d.data_fmt==FMT_UNSIGNED) {
    if (d.data_bpp==8)    add_from_data<uint8>( (uint8*)data, num_data_points, mask );
    else
    if (d.data_bpp == 16) add_from_data<uint16>( (uint16*)data, num_data_points, mask );
    else
    if (d.data_bpp == 32) add_from_data_scale<uint32>( (uint32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_SIGNED) {
    if (d.data_bpp==8)    add_from_data_scale<int8>( (int8*)data, num_data_points, mask );
    else
    if (d.data_bpp == 16) add_from_data_scale<int16>( (int16*)data, num_data_points, mask );
    else
    if (d.data_bpp == 32) add_from_data_scale<int32>( (int32*)data, num_data_points, mask );
  } else
  if (d.data_fmt==FMT_FLOAT) {
    if (d.data_bpp == 32) add_from_data_scale<float32>( (float32*)data, num_data_points, mask );
    else
    if (d.data_bpp == 64) add_from_data_scale<float64>( (float64*)data, num_data_points, mask );
    else
    if (d.data_bpp == 80) add_from_data_scale<float80>( (float80*)data, num_data_points, mask );
  }
}



int Histogram::bin_of_last_nonzero() const {
  for (int i=(int)hist.size()-1; i>=0; --i)
    if (hist[i] != 0)
      return i;
  return 0;
}

int Histogram::bin_of_first_nonzero() const {
  for (unsigned int i=0; i<hist.size(); ++i)
    if (hist[i] != 0)
      return i;
  return 0;
}

int Histogram::bin_of_max_value() const {
  int bin = 0;
  Histogram::StorageType val = hist[0];
  for (unsigned int i=0; i<hist.size(); ++i)
    if (hist[i] > val) {
      val = hist[i];
      bin = i;
    }
  return bin;
}

int Histogram::bin_of_min_value() const {
  int bin = 0;
  Histogram::StorageType val = hist[0];
  for (unsigned int i=0; i<hist.size(); ++i)
    if (hist[i] < val) {
      val = hist[i];
      bin = i;
    }
  return bin;
}

double Histogram::max_value() const {
  if (d.data_fmt==FMT_UNSIGNED && d.data_bpp<=16)
    return bin_of_last_nonzero();
  else
    return d.value_max;
}

double Histogram::min_value() const {
  if (d.data_fmt==FMT_UNSIGNED && d.data_bpp<=16)
    return bin_of_first_nonzero();
  else
    return d.value_min;
}

int Histogram::bin_number_nonzero() const {
  int unique = 0;
  for (unsigned int i=0; i<hist.size(); ++i) 
    if (hist[i] != 0) ++unique;
  return unique;
}

double Histogram::average() const {
  double a=0.0, s=0.0;
  for (unsigned int i=0; i<hist.size(); ++i)
    if (hist[i]>0) {
      a += i * hist[i];
      s += hist[i];
    }
  double mu = a/s;
  return (mu/d.scale) + d.shift;
}

double Histogram::std() const {
  double mu = average();
  double a=0.0, s=0.0;
  for (unsigned int i=0; i<hist.size(); ++i)
    if (hist[i]>0) {
      s += hist[i];
      a += (((double)i)-mu)*(((double)i)-mu) * hist[i];
    }
  double sig_sq = sqrt( a/(s-1) ); 
  return (sig_sq/d.scale) + d.shift;
}

Histogram::StorageType Histogram::cumsum( const unsigned int &bin ) const {
  unsigned int b = bin;
  if ( b >= hist.size() ) b = (unsigned int)hist.size()-1;
  Histogram::StorageType sum=0;
  for (unsigned int i=0; i<=b; ++i) 
    sum += hist[i];
  return sum;
}

Histogram::StorageType Histogram::get_value( const unsigned int &bin ) const {
  if ( bin < hist.size() )
    return hist[bin];
  else
    return 0;
}

void Histogram::set_value( const unsigned int &bin, const Histogram::StorageType &val ) {
  if ( bin < hist.size() )
    hist[bin] = val;
}

void Histogram::append_value( const unsigned int &bin, const Histogram::StorageType &val ) {
  if ( bin < hist.size() )
    hist[bin] += val;
}

//------------------------------------------------------------------------------
// I/O
//------------------------------------------------------------------------------

/*
Histogram binary content:
0x00 'BIM1' - 4 bytes header
0x04 'HST1' - 4 bytes spec
0x07        - XX bytes HistogramInternal
0xXX NUM    - 1xUINT32 number of elements in histogram vector
0xXX        - histogram vector Histogram::StorageType * NUM
*/

const char Histogram_mgk[4] = { 'B','I','M','1' };
const char Histogram_spc[4] = { 'H','S','T','1' };

bool Histogram::to(const std::string &fileName) {
  std::ofstream f( fileName.c_str(), std::ios_base::binary );
  return this->to(&f);
}

bool Histogram::to(std::ostream *s) {
  // write header
  s->write( Histogram_mgk, sizeof(Histogram_mgk) );
  s->write( Histogram_spc, sizeof(Histogram_spc) );
  s->write( (const char *) &d, sizeof(bim::HistogramInternal) );

  // write data
  uint32 sz = this->hist.size();
  s->write( (const char *) &sz, sizeof(uint32) );
  s->write( (const char *) &this->hist[0], sizeof(Histogram::StorageType)*this->hist.size() );
  s->flush();
  return true;
}

bool Histogram::from(const std::string &fileName) {
  std::ifstream f( fileName.c_str(), std::ios_base::binary  );
  return this->from(&f);
}

bool Histogram::from(std::istream *s) {
  // read header
  char hts_hdr[sizeof(Histogram_mgk)];
  char hts_spc[sizeof(Histogram_spc)];

  s->read( hts_hdr, sizeof(Histogram_mgk) );
  if (memcmp( hts_hdr, Histogram_mgk, sizeof(Histogram_mgk) )!=0) return false;

  s->read( hts_spc, sizeof(Histogram_spc) );
  if (memcmp( hts_spc, Histogram_spc, sizeof(Histogram_spc) )!=0) return false; 

  s->read( (char *) &d, sizeof(bim::HistogramInternal) );

  // read data
  uint32 sz;
  s->read( (char *) &sz, sizeof(uint32) );
  this->hist.resize(sz);
  
  s->read( (char *) &this->hist[0], sizeof(Histogram::StorageType)*this->hist.size() );
  return true;
}

inline void write_string(std::ostream *s, const std::string &str) {
    s->write( str.c_str(), str.size() );
}

inline std::string stringPixelType( const bim::DataFormat &pixelType ) {
  if (pixelType==bim::FMT_SIGNED) return "signed";
  if (pixelType==bim::FMT_FLOAT) return "float";
  return "unsigned";
}

bool Histogram::toXML(const std::string &fileName) {
    std::ofstream f( fileName.c_str(), std::ios_base::binary );
    write_string(&f, "<histogram name=\"channel\" value=\"0\">");
    this->toXML(&f);
    write_string(&f, "</histogram>");
    return true;
}

bool Histogram::toXML(std::ostream *s) {
    // write header
    write_string(s, xstring::xprintf("<tag name=\"data_bits_per_pixel\" value=\"%d\" />", this->d.data_bpp));
    write_string(s, xstring::xprintf("<tag name=\"data_format\" value=\"%s\" />", stringPixelType((bim::DataFormat) this->d.data_fmt).c_str()));
    write_string(s, xstring::xprintf("<tag name=\"data_value_min\" value=\"%f\" />", this->d.value_min));
    write_string(s, xstring::xprintf("<tag name=\"data_value_max\" value=\"%f\" />", this->d.value_max));
    write_string(s, xstring::xprintf("<tag name=\"data_scale\" value=\"%f\" />", this->d.scale));
    write_string(s, xstring::xprintf("<tag name=\"data_shift\" value=\"%f\" />", this->d.shift));

    // write data
    write_string(s, xstring::xprintf("<value>%d", this->hist[0]));
    for (uint32 i=1; i<this->hist.size(); ++i) {
        write_string(s, xstring::xprintf(",%d", this->hist[i]));
    }
    write_string(s, "</value>");

    s->flush();
    return true;
}

//******************************************************************************
// Lut
//******************************************************************************

Lut::Lut( ) {
  lut.resize( 0 ); 
  this->generator = linear_full_range_generator;
  type = ltLinearFullRange;
}

Lut::Lut( const Histogram &in, const Histogram &out, void *args ) {
  init( in, out, args );
}

Lut::Lut( const Histogram &in, const Histogram &out, const LutType &type, void *args ) {
  init( in, out, type, args );
}

Lut::Lut( const Histogram &in, const Histogram &out, LutGenerator custom_generator, void *args ) {
  init( in, out, custom_generator, args );
}

Lut::~Lut() {

}

void Lut::init( const Histogram &in, const Histogram &out, void *args ) {
  lut.resize( in.size() );
  clear( );
  h_in = in;
  h_out = out;
  if (generator) generator( in, lut, out.size(), args );
}

void Lut::init( const Histogram &in, const Histogram &out, const LutType &type, void *args ) {
  this->type = type;
  if (type == ltLinearFullRange)     generator = linear_full_range_generator;
  else
  if (type == ltLinearDataRange)     generator = linear_data_range_generator;
  else
  if (type == ltLinearDataTolerance) generator = linear_data_tolerance_generator;
  else
  if (type == ltEqualize)            generator = equalized_generator;
  else
  if (type == ltTypecast)            generator = typecast_generator;
  else
  if (type == ltFloat01)             generator = linear_float01_generator;
  else
  if (type == ltGamma)               generator = linear_gamma_generator;
  else
  if (type == ltMinMaxGamma)         generator = linear_min_max_gamma_generator;
  init( in, out, args );
}

void Lut::init( const Histogram &in, const Histogram &out, LutGenerator custom_generator, void *args ) {
  this->type = ltCustom;
  generator = custom_generator;
  init( in, out, args );
}

void Lut::clear( ) {
  lut = std::vector<Lut::StorageType>(lut.size(), 0); 
}

template <typename Tl>
void Lut::set_lut( const std::vector<Tl> &new_lut ) {
  lut.assign( new_lut.begin(), new_lut.end() );
}
template void Lut::set_lut<unsigned char>( const std::vector<unsigned char> &new_lut );
template void Lut::set_lut<unsigned int>( const std::vector<unsigned int> &new_lut );
template void Lut::set_lut<double>( const std::vector<double> &new_lut );

Lut::StorageType Lut::get_value( const unsigned int &pos ) const {
  if ( pos < lut.size() )
    return lut[pos];
  else
    return 0;
}

void Lut::set_value( const unsigned int &pos, const Lut::StorageType &val ) {
  if ( pos < lut.size() )
    lut[pos] = val;
}

template <typename Ti, typename To>
void Lut::apply_lut( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const {
  if (!ibuf || !obuf) return;
  if (this->type == ltTypecast) {
      this->apply_typecast( ibuf, obuf, num_data_points );
      return;
  }

  #pragma omp parallel for default(shared)
  for (bim::int64 i=0; i<num_data_points; ++i)
      obuf[i] = bim::trim<To, bim::Lut::StorageType>(lut[ibuf[i]]);
}

template <typename Ti, typename To>
void Lut::apply_lut_scale_from( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const {
  if (!ibuf || !obuf) return;
  if (this->type == ltTypecast) {
      this->apply_typecast( ibuf, obuf, num_data_points );
      return;
  }
  double scale = h_in.d.scale;
  double shift = h_in.d.shift;
  double range = (double)lut.size();

  #pragma omp parallel for default(shared)
  for (bim::int64 i=0; i<num_data_points; ++i)
    obuf[i] = bim::trim<To, bim::Lut::StorageType>(lut[ bim::trim<unsigned int, double>( ((double)ibuf[i]-shift)*scale, 0, range-1) ]);
}

template <typename Ti, typename To>
void Lut::apply_typecast( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const {
  if (!ibuf || !obuf) return;

  #pragma omp parallel for default(shared)
  for (bim::int64 i=0; i<num_data_points; ++i)
    obuf[i] = (To) ibuf[i];
}

//------------------------------------------------------------------------------------
// ok, follows crazy code bloat of instantiations, will change this eventually
//------------------------------------------------------------------------------------

// this guy instantiates real method based on input template
template <typename Ti>
inline void Lut::do_apply_lut( const Ti *ibuf, const void *obuf, const unsigned int &num_data_points ) const {
  
  if (h_out.dataBpp()==8 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut<Ti, uint8>( ibuf, (uint8*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==16 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut<Ti, uint16>( ibuf, (uint16*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==32 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut<Ti, uint32>( ibuf, (uint32*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==32 && h_out.dataFormat()==FMT_FLOAT )
    apply_lut<Ti, float32>( ibuf, (float32*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==64 && h_out.dataFormat()==FMT_FLOAT )
    apply_lut<Ti, float64>( ibuf, (float64*) obuf, num_data_points );
}

// this guy instantiates real method based on input template
template <typename Ti>
inline void Lut::do_apply_lut_scale_from( const Ti *ibuf, const void *obuf, const unsigned int &num_data_points ) const {
  if (h_out.dataBpp()==8 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut_scale_from<Ti, uint8>( ibuf, (uint8*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==16 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut_scale_from<Ti, uint16>( ibuf, (uint16*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==32 && h_out.dataFormat()!=FMT_FLOAT )
    apply_lut_scale_from<Ti, uint32>( ibuf, (uint32*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==32 && h_out.dataFormat()==FMT_FLOAT )
    apply_lut_scale_from<Ti, float32>( ibuf, (float32*) obuf, num_data_points );
  else
  if (h_out.dataBpp()==64 && h_out.dataFormat()==FMT_FLOAT )
    apply_lut_scale_from<Ti, float64>( ibuf, (float64*) obuf, num_data_points );
}

void Lut::apply( void *ibuf, const void *obuf, const unsigned int &num_data_points ) const {
  if (lut.size() <= 0) return;

  // uint
  if (h_in.dataBpp()==8 && h_in.dataFormat()==FMT_UNSIGNED)
    do_apply_lut<uint8>( (uint8*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==16 && h_in.dataFormat()==FMT_UNSIGNED)
    do_apply_lut<uint16>( (uint16*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==32 && h_in.dataFormat()==FMT_UNSIGNED)
    do_apply_lut_scale_from<uint32>( (uint32*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==64 && h_in.dataFormat()==FMT_UNSIGNED)
    do_apply_lut_scale_from<uint64>( (uint64*) ibuf, obuf, num_data_points );
  else
  // int
  if (h_in.dataBpp()==8 && h_in.dataFormat()==FMT_SIGNED)
    do_apply_lut_scale_from<int8>( (int8*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==16 && h_in.dataFormat()==FMT_SIGNED)
    do_apply_lut_scale_from<int16>( (int16*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==32 && h_in.dataFormat()==FMT_SIGNED)
    do_apply_lut_scale_from<int32>( (int32*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==64 && h_in.dataFormat()==FMT_SIGNED)
    do_apply_lut_scale_from<int64>( (int64*) ibuf, obuf, num_data_points );
  else
  // float: current implementation would provide poor quality for float2float because of the LUT binning size
  // should look into doing this by applying generator function for each element of the data ignoring LUT at all...
  if (h_in.dataBpp()==32 && h_in.dataFormat()==FMT_FLOAT)
    do_apply_lut_scale_from<float32>( (float32*) ibuf, obuf, num_data_points );
  else
  if (h_in.dataBpp()==64 && h_in.dataFormat()==FMT_FLOAT)
    do_apply_lut_scale_from<float64>( (float64*) ibuf, obuf, num_data_points );
}

void Lut::apply( const Histogram &in, Histogram &out ) const {
  if (lut.size() <= 0) return;
  out.clear();

  if (this->type==ltTypecast) {
      out = in;
      return;
  }

  for (unsigned int i=0; i<in.size(); ++i)
      out.append_value( (unsigned int) lut[i], in[i] );
}

//******************************************************************************
// Generators
//******************************************************************************

//------------------------
// this is not a generator per se
//------------------------
template <typename Tl>
void bim::linear_range_generator( int b, int e, unsigned int out_phys_range, std::vector<Tl> &lut ) {
  
  //if (lut.size() < out_phys_range) lut.resize(out_phys_range);
  // simple linear mapping for actual range
  double range = e - b;
  if (range < 1) range = out_phys_range;
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double> ( (((double)x)-b)*out_phys_range/range, 0, out_phys_range-1);
}
template void bim::linear_range_generator<unsigned char>( int b, int e, unsigned int out_phys_range, std::vector<unsigned char> &lut );
template void bim::linear_range_generator<unsigned int>( int b, int e, unsigned int out_phys_range, std::vector<unsigned int> &lut );
template void bim::linear_range_generator<double>( int b, int e, unsigned int out_phys_range, std::vector<double> &lut );
//------------------------


template <typename Tl>
void bim::linear_full_range_generator( const Histogram &, std::vector<Tl> &lut, unsigned int out_phys_range, void * ) {
  // simple linear mapping for full range
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = (Tl) ( (((double)x)*(out_phys_range-1.0))/(lut.size()-1.0) );
}
template void bim::linear_full_range_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_full_range_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_full_range_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void bim::linear_data_range_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void * ) {
  // simple linear mapping for actual range
  double b = in.first_pos();
  double e = in.last_pos();
  double range = e - b;
  if (range < 1) range = out_phys_range;
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double> ( (((double)x)-b)*out_phys_range/range, 0, out_phys_range-1);
}
template void bim::linear_data_range_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_data_range_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_data_range_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void bim::linear_data_tolerance_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args ) {
    double tolerance = 1.0;
    if (args)
        tolerance = *(double *)args;

  // simple linear mapping cutting elements with small appearence
  // get 1% threshold
  unsigned int th = (unsigned int) ( in[ in.bin_of_max_value() ] * tolerance / 100.0 );
  double b = 0;
  double e = in.size()-1;
  for (unsigned int x=0; x<in.size(); ++x)
    if ( in[x] > th ) {
      b = x;
      break;
    }
  for (int x=in.size()-1; x>=0; --x)
    if ( in[x] > th ) {
      e = x;
      break;
    }

  double range = e - b;
  if (range < 1) range = out_phys_range;
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double> ( (((double)x)-b)*out_phys_range/range, 0, out_phys_range-1 );
}
template void bim::linear_data_tolerance_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_data_tolerance_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_data_tolerance_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void bim::equalized_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void * ) {
  
  // equalize
  std::vector<double> map(lut.size(), 0);
  map[0] = (double) in[0];
  for (unsigned int x=1; x<lut.size(); ++x)
    map[x] = map[x-1] + in[x];

  double div = map[lut.size()-1]-map[0];
  if (div > 0)
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double>( (Tl) (out_phys_range-1.0)*((map[x]-map[0]) / div), 0, out_phys_range-1 );
}
template void bim::equalized_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::equalized_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::equalized_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void bim::linear_custom_range_generator( const Histogram &/*in*/, std::vector<Tl> &lut, unsigned int out_phys_range, void *args ) {
  // simple linear mapping for actual range
  int *vals = (int *)args;
  double b = vals[0];
  double e = vals[1];
  double range = e - b;
  if (range < 1) range = out_phys_range;
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double> ( (((double)x)-b)*out_phys_range/range, 0, out_phys_range-1);
}
template void bim::linear_custom_range_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_custom_range_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_custom_range_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

// typecast_generator simply indicates that application of the LUT will produce typecasted output
template <typename Tl>
void bim::typecast_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void * ) {
    // nothing here really, this is just a place holder for typcasted copy
}
template void bim::typecast_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::typecast_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::typecast_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );

// linear_float01 generates LUT to produce float in the range of 0-1
template <typename Tl>
void bim::linear_float01_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void * ) {
  out_phys_range = 1;
  double b = in.first_pos();
  double e = in.last_pos();
  double range = e - b;
  if (range < 1) range = out_phys_range;
  for (unsigned int x=0; x<lut.size(); ++x)
    lut[x] = bim::trim<Tl, double> ( (((double)x)-b)*out_phys_range/range, 0, out_phys_range-1);
}
template void bim::linear_float01_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_float01_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_float01_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );


template <typename Tl>
void bim::linear_gamma_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args ) {
    double gamma = bim::trim<double, double>( *(double *)args, 0, std::numeric_limits<double>::max() );
    gamma = 1.0/gamma;
    double b = pow((double)in.first_pos(), gamma); 
    double e = pow((double)in.last_pos(), gamma); 
    double range = e - b;
    if (range < 1) range = out_phys_range;
    for (unsigned int x=0; x<lut.size(); ++x) {
        double vpx = (pow((double)x, gamma)-b)*out_phys_range/range;
        lut[x] = bim::trim<Tl, double> (vpx, 0, out_phys_range-1);
    }
}
template void bim::linear_gamma_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_gamma_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_gamma_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );


template <typename Tl>
void bim::linear_min_max_gamma_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args ) {
    bim::min_max_gamma_args arg = * (bim::min_max_gamma_args *) args;
    double gamma = bim::trim<double, double>( arg.gamma, 0, std::numeric_limits<double>::max() );
    gamma = 1.0/gamma;
    double b = pow(arg.minv, gamma);
    double e = pow(arg.maxv, gamma);
    double range = e - b;
    if (range < 1) range = out_phys_range;
    for (unsigned int x=0; x<lut.size(); ++x) {
        double vpx = (pow((double)x, gamma)-b)*out_phys_range/range;
        lut[x] = bim::trim<Tl, double> (vpx, 0, out_phys_range-1);
    }
}
template void bim::linear_min_max_gamma_generator<unsigned char>( const Histogram &in, std::vector<unsigned char> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_min_max_gamma_generator<unsigned int>( const Histogram &in, std::vector<unsigned int> &lut, unsigned int out_phys_range, void *args );
template void bim::linear_min_max_gamma_generator<double>( const Histogram &in, std::vector<double> &lut, unsigned int out_phys_range, void *args );
