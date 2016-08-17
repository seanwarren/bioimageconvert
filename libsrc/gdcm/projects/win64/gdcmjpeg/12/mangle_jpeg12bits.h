/* mangle_jpeg12bits.h file generated by CMake http://www.cmake.org */
#ifndef gdcmjpeg12_mangle_h
#define gdcmjpeg12_mangle_h

/*
 * This header file mangles all symbols exported from the jpeg library.
 * It is included in all files while building the jpeg library.  Due to
 * namespace pollution, no jpeg headers should be included in .h files in
 * GDCM. This also allows us to generate two jpeg library (8bits and 12bits).
 *
 * The following command was used to obtain the symbol list:
 *
 * nm libgdcmjpeg12.a | grep " [R|T] " | colrm 1 11 | sort
 */


#define jcopy_block_row gdcmjpeg12_jcopy_block_row
#define jcopy_sample_rows gdcmjpeg12_jcopy_sample_rows
#define jdiv_round_up gdcmjpeg12_jdiv_round_up
#define jinit_1pass_quantizer gdcmjpeg12_jinit_1pass_quantizer
#define jinit_2pass_quantizer gdcmjpeg12_jinit_2pass_quantizer
#define jinit_arith_decoder gdcmjpeg12_jinit_arith_decoder
#define jinit_arith_encoder gdcmjpeg12_jinit_arith_encoder
#define jinit_c_codec gdcmjpeg12_jinit_c_codec
#define jinit_c_coef_controller gdcmjpeg12_jinit_c_coef_controller
#define jinit_c_diff_controller gdcmjpeg12_jinit_c_diff_controller
#define jinit_c_main_controller gdcmjpeg12_jinit_c_main_controller
#define jinit_c_master_control gdcmjpeg12_jinit_c_master_control
#define jinit_c_prep_controller gdcmjpeg12_jinit_c_prep_controller
#define jinit_c_scaler gdcmjpeg12_jinit_c_scaler
#define jinit_color_converter gdcmjpeg12_jinit_color_converter
#define jinit_color_deconverter gdcmjpeg12_jinit_color_deconverter
#define jinit_compress_master gdcmjpeg12_jinit_compress_master
#define jinit_d_codec gdcmjpeg12_jinit_d_codec
#define jinit_d_coef_controller gdcmjpeg12_jinit_d_coef_controller
#define jinit_d_diff_controller gdcmjpeg12_jinit_d_diff_controller
#define jinit_d_main_controller gdcmjpeg12_jinit_d_main_controller
#define jinit_d_post_controller gdcmjpeg12_jinit_d_post_controller
#define jinit_d_scaler gdcmjpeg12_jinit_d_scaler
#define jinit_differencer gdcmjpeg12_jinit_differencer
#define jinit_downsampler gdcmjpeg12_jinit_downsampler
#define jinit_forward_dct gdcmjpeg12_jinit_forward_dct
#define jinit_input_controller gdcmjpeg12_jinit_input_controller
#define jinit_inverse_dct gdcmjpeg12_jinit_inverse_dct
#define jinit_lhuff_decoder gdcmjpeg12_jinit_lhuff_decoder
#define jinit_lhuff_encoder gdcmjpeg12_jinit_lhuff_encoder
#define jinit_lossless_c_codec gdcmjpeg12_jinit_lossless_c_codec
#define jinit_lossless_d_codec gdcmjpeg12_jinit_lossless_d_codec
#define jinit_lossy_c_codec gdcmjpeg12_jinit_lossy_c_codec
#define jinit_lossy_d_codec gdcmjpeg12_jinit_lossy_d_codec
#define jinit_marker_reader gdcmjpeg12_jinit_marker_reader
#define jinit_marker_writer gdcmjpeg12_jinit_marker_writer
#define jinit_master_decompress gdcmjpeg12_jinit_master_decompress
#define jinit_memory_mgr gdcmjpeg12_jinit_memory_mgr
#define jinit_merged_upsampler gdcmjpeg12_jinit_merged_upsampler
#define jinit_phuff_decoder gdcmjpeg12_jinit_phuff_decoder
#define jinit_phuff_encoder gdcmjpeg12_jinit_phuff_encoder
#define jinit_shuff_decoder gdcmjpeg12_jinit_shuff_decoder
#define jinit_shuff_encoder gdcmjpeg12_jinit_shuff_encoder
#define jinit_undifferencer gdcmjpeg12_jinit_undifferencer
#define jinit_upsampler gdcmjpeg12_jinit_upsampler
#define jpeg_CreateCompress gdcmjpeg12_jpeg_CreateCompress
#define jpeg_CreateDecompress gdcmjpeg12_jpeg_CreateDecompress
#define jpeg_abort gdcmjpeg12_jpeg_abort
#define jpeg_abort_compress gdcmjpeg12_jpeg_abort_compress
#define jpeg_abort_decompress gdcmjpeg12_jpeg_abort_decompress
#define jpeg_add_quant_table gdcmjpeg12_jpeg_add_quant_table
#define jpeg_alloc_huff_table gdcmjpeg12_jpeg_alloc_huff_table
#define jpeg_alloc_quant_table gdcmjpeg12_jpeg_alloc_quant_table
#define jpeg_calc_output_dimensions gdcmjpeg12_jpeg_calc_output_dimensions
#define jpeg_consume_input gdcmjpeg12_jpeg_consume_input
#define jpeg_copy_critical_parameters gdcmjpeg12_jpeg_copy_critical_parameters
#define jpeg_default_colorspace gdcmjpeg12_jpeg_default_colorspace
#define jpeg_destroy gdcmjpeg12_jpeg_destroy
#define jpeg_destroy_compress gdcmjpeg12_jpeg_destroy_compress
#define jpeg_destroy_decompress gdcmjpeg12_jpeg_destroy_decompress
#define jpeg_fdct_float gdcmjpeg12_jpeg_fdct_float
#define jpeg_fdct_ifast gdcmjpeg12_jpeg_fdct_ifast
#define jpeg_fdct_islow gdcmjpeg12_jpeg_fdct_islow
#define jpeg_fill_bit_buffer gdcmjpeg12_jpeg_fill_bit_buffer
#define jpeg_finish_compress gdcmjpeg12_jpeg_finish_compress
#define jpeg_finish_decompress gdcmjpeg12_jpeg_finish_decompress
#define jpeg_finish_output gdcmjpeg12_jpeg_finish_output
#define jpeg_free_large gdcmjpeg12_jpeg_free_large
#define jpeg_free_small gdcmjpeg12_jpeg_free_small
#define jpeg_gen_optimal_table gdcmjpeg12_jpeg_gen_optimal_table
#define jpeg_get_large gdcmjpeg12_jpeg_get_large
#define jpeg_get_small gdcmjpeg12_jpeg_get_small
#define jpeg_has_multiple_scans gdcmjpeg12_jpeg_has_multiple_scans
#define jpeg_huff_decode gdcmjpeg12_jpeg_huff_decode
#define jpeg_idct_1x1 gdcmjpeg12_jpeg_idct_1x1
#define jpeg_idct_2x2 gdcmjpeg12_jpeg_idct_2x2
#define jpeg_idct_4x4 gdcmjpeg12_jpeg_idct_4x4
#define jpeg_idct_float gdcmjpeg12_jpeg_idct_float
#define jpeg_idct_ifast gdcmjpeg12_jpeg_idct_ifast
#define jpeg_idct_islow gdcmjpeg12_jpeg_idct_islow
#define jpeg_input_complete gdcmjpeg12_jpeg_input_complete
#define jpeg_make_c_derived_tbl gdcmjpeg12_jpeg_make_c_derived_tbl
#define jpeg_make_d_derived_tbl gdcmjpeg12_jpeg_make_d_derived_tbl
#define jpeg_mem_available gdcmjpeg12_jpeg_mem_available
#define jpeg_mem_init gdcmjpeg12_jpeg_mem_init
#define jpeg_mem_term gdcmjpeg12_jpeg_mem_term
#define jpeg_natural_order gdcmjpeg12_jpeg_natural_order
#define jpeg_new_colormap gdcmjpeg12_jpeg_new_colormap
#define jpeg_open_backing_store gdcmjpeg12_jpeg_open_backing_store
#define jpeg_quality_scaling gdcmjpeg12_jpeg_quality_scaling
#define jpeg_read_coefficients gdcmjpeg12_jpeg_read_coefficients
#define jpeg_read_header gdcmjpeg12_jpeg_read_header
#define jpeg_read_raw_data gdcmjpeg12_jpeg_read_raw_data
#define jpeg_read_scanlines gdcmjpeg12_jpeg_read_scanlines
#define jpeg_resync_to_restart gdcmjpeg12_jpeg_resync_to_restart
#define jpeg_save_markers gdcmjpeg12_jpeg_save_markers
#define jpeg_set_colorspace gdcmjpeg12_jpeg_set_colorspace
#define jpeg_set_defaults gdcmjpeg12_jpeg_set_defaults
#define jpeg_set_linear_quality gdcmjpeg12_jpeg_set_linear_quality
#define jpeg_set_marker_processor gdcmjpeg12_jpeg_set_marker_processor
#define jpeg_set_quality gdcmjpeg12_jpeg_set_quality
#define jpeg_simple_lossless gdcmjpeg12_jpeg_simple_lossless
#define jpeg_simple_progression gdcmjpeg12_jpeg_simple_progression
#define jpeg_start_compress gdcmjpeg12_jpeg_start_compress
#define jpeg_start_decompress gdcmjpeg12_jpeg_start_decompress
#define jpeg_start_output gdcmjpeg12_jpeg_start_output
#define jpeg_std_error gdcmjpeg12_jpeg_std_error
#define jpeg_std_message_table gdcmjpeg12_jpeg_std_message_table
#define jpeg_stdio_dest gdcmjpeg12_jpeg_stdio_dest
#define jpeg_stdio_src gdcmjpeg12_jpeg_stdio_src
#define jpeg_suppress_tables gdcmjpeg12_jpeg_suppress_tables
#define jpeg_write_coefficients gdcmjpeg12_jpeg_write_coefficients
#define jpeg_write_m_byte gdcmjpeg12_jpeg_write_m_byte
#define jpeg_write_m_header gdcmjpeg12_jpeg_write_m_header
#define jpeg_write_marker gdcmjpeg12_jpeg_write_marker
#define jpeg_write_raw_data gdcmjpeg12_jpeg_write_raw_data
#define jpeg_write_scanlines gdcmjpeg12_jpeg_write_scanlines
#define jpeg_write_tables gdcmjpeg12_jpeg_write_tables
#define jround_up gdcmjpeg12_jround_up
#define jzero_far gdcmjpeg12_jzero_far

#define jpeg_memory_src gdcmjpeg12_memory_src

#endif
