/*****************************************************************************
Miscelaneous functions
Copyright (c) 2013, Center for Bio-Image Informatics, UCSB
Copyright (c) 2013, Dmitry Fedorov <www.dimin.net> <dima@dimin.net>

Author: Dmitry Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>

History:
2013-01-12 14:13:40 - First creation

ver : 1
*****************************************************************************/

#ifndef BIM_FORMATS_MISC_H
#define BIM_FORMATS_MISC_H

template <typename T>
void copy_sample_interleaved_to_planar(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out, int stride = 0) {
    T *raw = (T *)in + sample;
    T *p = (T *)out;
    int step = samples;
    size_t inrowsz = stride == 0 ? samples*W : stride;
    size_t ourowsz = W;

#pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W > BIM_OMP_FOR2 && H > BIM_OMP_FOR2)
    for (bim::int64 y = 0; y < H; ++y) {
        T *lin = raw + y*inrowsz;
        T *lou = p + y*ourowsz;
        for (bim::int64 x = 0; x < W; ++x) {
            *lou = *lin;
            lou++;
            lin += step;
        } // for x
    } // for y
}

template <typename T>
void copy_sample_planar_to_interleaved(bim::uint64 W, bim::uint64 H, int samples, int sample, const void *in, void *out) {
    T *raw = (T *)in;
    T *p = (T *)out + sample;
    int step = samples;
    size_t inrowsz = W;
    size_t ourowsz = samples*W;

#pragma omp parallel for default(shared) BIM_OMP_SCHEDULE if (W > BIM_OMP_FOR2 && H > BIM_OMP_FOR2)
    for (bim::int64 y = 0; y < H; ++y) {
        T *lin = raw + y*inrowsz;
        T *lou = p + y*ourowsz;
        for (bim::int64 x = 0; x < W; ++x) {
            *lou = *lin;
            lin++;
            lou += step;
        } // for x
    } // for y
}


#endif // BIM_FORMATS_MISC_H
