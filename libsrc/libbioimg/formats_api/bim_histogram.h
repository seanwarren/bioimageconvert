/*******************************************************************************

  histogram and lut
  
  Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

  History:
    10/20/2006 20:21 - First creation
    2007-06-26 12:18 - added lut class
    2010-01-22 17:06 - changed interface to support floating point data

  ver: 3
        
*******************************************************************************/

#ifndef BIM_HISTOGRAM_H
#define BIM_HISTOGRAM_H

#include <vector>
#include <iostream>
#include <fstream>

#include "bim_img_format_interface.h"

//******************************************************************************
// Histogram
//******************************************************************************

#define BIM_HISTOGRAM_VERSION 1

namespace bim {

#pragma pack(push, 1)
struct HistogramInternal {
  uint16 data_bpp; // bits per pixel  
  uint16 data_fmt; // signed, unsigned, float
  double shift;
  double scale;
  double value_min; 
  double value_max;
};
#pragma pack(pop)



class Lut;
class Histogram {
  public:

    enum ChannelMode { 
      cmSeparate=0, 
      cmCombined=1, 
    };

    typedef uint64 StorageType; 
    static const unsigned int defaultSize=256; // 256, 65536

    Histogram( const unsigned int &bpp=0, const DataFormat &fmt=FMT_UNSIGNED );
    Histogram( const unsigned int &bpp, void *data, const unsigned int &num_data_points, const DataFormat &fmt=FMT_UNSIGNED, unsigned char *mask=0 );
    ~Histogram();

    void newData( const unsigned int &bpp, void *data, const unsigned int &num_data_points, const DataFormat &fmt=FMT_UNSIGNED, unsigned char *mask=0 );
    void clear();

    void init( const unsigned int &bpp, const DataFormat &fmt=FMT_UNSIGNED );
    void updateStats( void *data, const unsigned int &num_data_points, unsigned char *mask=0 );
    // careful with this function operating on data with 32 bits and above, stats should be properly updated for all data chunks if vary
    // use the updateStats method on each data chunk prior to calling addData on >=32bit data
    void addData( void *data, const unsigned int &num_data_points, unsigned char *mask=0 );

    unsigned int getDefaultSize() const { return default_size; }
    void         setDefaultSize(const unsigned int &v) { default_size=v; }

    bool         isValid()    const { return d.data_bpp>0; }
    unsigned int dataBpp()    const { return d.data_bpp; }
    DataFormat dataFormat() const { return (DataFormat) d.data_fmt; }

    int bin_of_max_value() const;
    int bin_of_min_value() const;
    double max_value() const;
    double min_value() const;

    int bin_of_first_nonzero() const;
    int bin_of_last_nonzero()  const;
    int bin_number_nonzero()   const;
    int first_pos()  const { return bin_of_first_nonzero(); }
    int last_pos()   const { return bin_of_last_nonzero(); }
    int num_unique() const { return bin_number_nonzero(); }

    inline unsigned int bin_number() const { return size(); }
    inline unsigned int size() const { return (unsigned int) hist.size(); }
    const std::vector<StorageType> &get_hist( ) const { return hist; }

    double average() const;
    double std() const;
    StorageType cumsum( const unsigned int &bin ) const;

    double get_shift() const { return d.shift; }
    double get_scale() const { return d.scale; }

    StorageType get_value( const unsigned int &bin ) const;
    void set_value( const unsigned int &bin, const StorageType &val );
    void append_value( const unsigned int &bin, const StorageType &val );

    inline StorageType operator[](unsigned int x) const { return hist[x]; }

    template <typename T>
    inline unsigned int bin_from( const T &data_point ) const;

  public:
    // I/O
    bool to(const std::string &fileName);
    bool to(std::ostream *s);
    bool from(const std::string &fileName);
    bool from(std::istream *s);
    bool toXML(const std::string &fileName);
    bool toXML(std::ostream *s);

  protected:
    int default_size;
    bool reversed_min_max;
    bim::HistogramInternal d;
    std::vector<StorageType> hist;

    template <typename T> 
    void init_stats();

    void initStats();
    void getStats( void *data, const unsigned int &num_data_points, unsigned char *mask=0 );

    inline void recompute_shift_scale();

    template <typename T>
    void get_data_stats( T *data, const unsigned int &num_data_points, unsigned char *mask=0 );

    template <typename T>
    void update_data_stats( T *data, const unsigned int &num_data_points, unsigned char *mask=0 );

    template <typename T>
    void add_from_data( T *data, const unsigned int &num_data_points, unsigned char *mask=0 );
    
    template <typename T>
    void add_from_data_scale( T *data, const unsigned int &num_data_points, unsigned char *mask=0 );



    friend class Lut;

};

//******************************************************************************
// Lut
//******************************************************************************

class Lut {
  public:

    enum LutType { 
      ltLinearFullRange=0, 
      ltLinearDataRange=1, 
      ltLinearDataTolerance=2,
      ltEqualize=3,
      ltTypecast=4,
      ltFloat01=5,
      ltGamma=6,
      ltMinMaxGamma=7,
      ltCustom=8, 
    };

    typedef double StorageType; 
    typedef void (*LutGenerator)( const Histogram &in, std::vector<StorageType> &lut, unsigned int out_phys_range, void *args );

    Lut( );
    Lut( const Histogram &in, const Histogram &out, void *args=NULL );
    Lut( const Histogram &in, const Histogram &out, const LutType &type, void *args=NULL );
    Lut( const Histogram &in, const Histogram &out, LutGenerator custom_generator, void *args=NULL );
    ~Lut();

    void init( const Histogram &in, const Histogram &out, void *args=NULL );
    void init( const Histogram &in, const Histogram &out, const LutType &type, void *args=NULL );
    void init( const Histogram &in, const Histogram &out, LutGenerator custom_generator, void *args=NULL );
    void clear( );

    unsigned int size() const { return (unsigned int) lut.size(); }
    const std::vector<StorageType> &get_lut( ) const { return lut; }

    int depthInput()  const { return h_in.dataBpp(); }
    int depthOutput() const { return h_out.dataBpp(); }
    DataFormat dataFormatInput()  const { return h_in.dataFormat(); }
    DataFormat dataFormatOutput() const { return h_out.dataFormat(); }

    template <typename Tl>
    void         set_lut( const std::vector<Tl> & );
    
    StorageType   get_value( const unsigned int &pos ) const;
    void          set_value( const unsigned int &pos, const StorageType &val );
    inline StorageType operator[](unsigned int x) const { return lut[x]; }

    void apply( void *ibuf, const void *obuf, const unsigned int &num_data_points ) const;
    // generates values of output histogram, given the current lut and in histogram
    void apply( const Histogram &in, Histogram &out ) const;

  protected:
    std::vector<StorageType> lut;
    LutGenerator generator;
    LutType type;
    Histogram h_in, h_out;
    void *internal_arguments;

    template <typename Ti, typename To>
    void apply_lut( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const;

    template <typename Ti, typename To>
    void apply_lut_scale_from( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const;

    template <typename Ti, typename To>
    void apply_typecast( const Ti *ibuf, To *obuf, const unsigned int &num_data_points ) const;

    template <typename Ti>
    inline void do_apply_lut( const Ti *ibuf, const void *obuf, const unsigned int &num_data_points ) const;

    template <typename Ti>
    inline void do_apply_lut_scale_from( const Ti *ibuf, const void *obuf, const unsigned int &num_data_points ) const;
};

//******************************************************************************
// Generators - default
//******************************************************************************

// misc
template <typename Tl>
void linear_range_generator( int b, int e, unsigned int out_phys_range, std::vector<Tl> &lut );

template <typename Tl>
void linear_full_range_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void linear_data_range_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void linear_data_tolerance_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void equalized_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

// args = int* (with 2 values)
template <typename Tl>
void linear_custom_range_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void typecast_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

template <typename Tl>
void linear_float01_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

// args = double*
template <typename Tl>
void linear_gamma_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );


struct min_max_gamma_args {
    double gamma;
    double minv;
    double maxv;
};

// args = min_max_gamma_args*
template <typename Tl>
void linear_min_max_gamma_generator( const Histogram &in, std::vector<Tl> &lut, unsigned int out_phys_range, void *args );

} // namespace bim

#endif //BIM_HISTOGRAM_H


