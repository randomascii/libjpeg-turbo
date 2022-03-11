/*
 * jpeg12int.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 1997-2009 by Guido Vollbeding.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2015-2016, 2019, 2021-2022, D. R. Commander.
 * Copyright (C) 2015, Google, Inc.
 * Copyright (C) 2021, Alex Richardson.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file provides common declarations for the various JPEG modules.
 * These declarations are considered internal to the JPEG library; most
 * applications using the library shouldn't need to include this file.
 */

#ifndef JPEG12INT_H
#define JPEG12INT_H


#ifndef JPEGINT_H

/* Declarations for both compression & decompression */

typedef enum {            /* Operating modes for buffer controllers */
  JBUF_PASS_THRU,         /* Plain stripwise operation */
  /* Remaining modes require a full-image buffer to have been created */
  JBUF_SAVE_SOURCE,       /* Run source subobject only, save output */
  JBUF_CRANK_DEST,        /* Run dest subobject only, using saved data */
  JBUF_SAVE_AND_PASS      /* Run both subobjects, save output */
} J_BUF_MODE;

/* Values of global_state field (jdapi.c has some dependencies on ordering!) */
#define CSTATE_START     100    /* after create_compress */
#define CSTATE_SCANNING  101    /* start_compress done, write_scanlines OK */
#define CSTATE_RAW_OK    102    /* start_compress done, write_raw_data OK */
#define CSTATE_WRCOEFS   103    /* jpeg12_write_coefficients done */
#define DSTATE_START     200    /* after create_decompress */
#define DSTATE_INHEADER  201    /* reading header markers, no SOS yet */
#define DSTATE_READY     202    /* found SOS, ready for start_decompress */
#define DSTATE_PRELOAD   203    /* reading multiscan file in start_decompress*/
#define DSTATE_PRESCAN   204    /* performing dummy pass for 2-pass quant */
#define DSTATE_SCANNING  205    /* start_decompress done, read_scanlines OK */
#define DSTATE_RAW_OK    206    /* start_decompress done, read_raw_data OK */
#define DSTATE_BUFIMAGE  207    /* expecting jpeg12_start_output */
#define DSTATE_BUFPOST   208    /* looking for SOS/EOI in
                                   jpeg12_finish_output */
#define DSTATE_RDCOEFS   209    /* reading file in jpeg12_read_coefficients */
#define DSTATE_STOPPING  210    /* looking for EOI in
                                   jpeg12_finish_decompress */


/* JLONG must hold at least signed 32-bit values. */
typedef long JLONG;

/* JUINTPTR must hold pointer values. */
#ifdef __UINTPTR_TYPE__
/*
 * __UINTPTR_TYPE__ is GNU-specific and available in GCC 4.6+ and Clang 3.0+.
 * Fortunately, that is sufficient to support the few architectures for which
 * sizeof(void *) != sizeof(size_t).  The only other options would require C99
 * or Clang-specific builtins.
 */
typedef __UINTPTR_TYPE__ JUINTPTR;
#else
typedef size_t JUINTPTR;
#endif

/*
 * Left shift macro that handles a negative operand without causing any
 * sanitizer warnings
 */

#define LEFT_SHIFT(a, b)  ((JLONG)((unsigned long)(a) << (b)))

#endif /* JPEGINT_H */


/* Declarations for compression modules */

/* Master control module */
struct jpeg12_comp_master {
  void (*prepare_for_pass) (j12_compress_ptr cinfo);
  void (*pass_startup) (j12_compress_ptr cinfo);
  void (*finish_pass) (j12_compress_ptr cinfo);

  /* State variables made visible to other modules */
  boolean call_pass_startup;    /* True if pass_startup must be called */
  boolean is_last_pass;         /* True during last pass */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg12_c_main_controller {
  void (*start_pass) (j12_compress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*process_data) (j12_compress_ptr cinfo, J12SAMPARRAY input_buf,
                        JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail);
};

/* Compression preprocessing (downsampling input buffer control) */
struct jpeg12_c_prep_controller {
  void (*start_pass) (j12_compress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*pre_process_data) (j12_compress_ptr cinfo, J12SAMPARRAY input_buf,
                            JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail,
                            J12SAMPIMAGE output_buf,
                            JDIMENSION *out_row_group_ctr,
                            JDIMENSION out_row_groups_avail);
};

/* Coefficient buffer control */
struct jpeg12_c_coef_controller {
  void (*start_pass) (j12_compress_ptr cinfo, J_BUF_MODE pass_mode);
  boolean (*compress_data) (j12_compress_ptr cinfo, J12SAMPIMAGE input_buf);
};

/* Colorspace conversion */
struct jpeg12_color_converter {
  void (*start_pass) (j12_compress_ptr cinfo);
  void (*color_convert) (j12_compress_ptr cinfo, J12SAMPARRAY input_buf,
                         J12SAMPIMAGE output_buf, JDIMENSION output_row,
                         int num_rows);
};

/* Downsampling */
struct jpeg12_downsampler {
  void (*start_pass) (j12_compress_ptr cinfo);
  void (*downsample) (j12_compress_ptr cinfo, J12SAMPIMAGE input_buf,
                      JDIMENSION in_row_index, J12SAMPIMAGE output_buf,
                      JDIMENSION out_row_group_index);

  boolean need_context_rows;    /* TRUE if need rows above & below */
};

/* Forward DCT (also controls coefficient quantization) */
struct jpeg12_forward_dct {
  void (*start_pass) (j12_compress_ptr cinfo);
  /* perhaps this should be an array??? */
  void (*forward_DCT) (j12_compress_ptr cinfo, jpeg_component_info *compptr,
                       J12SAMPARRAY sample_data, JBLOCKROW coef_blocks,
                       JDIMENSION start_row, JDIMENSION start_col,
                       JDIMENSION num_blocks);
};

/* Entropy encoding */
struct jpeg12_entropy_encoder {
  void (*start_pass) (j12_compress_ptr cinfo, boolean gather_statistics);
  boolean (*encode_mcu) (j12_compress_ptr cinfo, JBLOCKROW *MCU_data);
  void (*finish_pass) (j12_compress_ptr cinfo);
};

/* Marker writing */
struct jpeg12_marker_writer {
  void (*write_file_header) (j12_compress_ptr cinfo);
  void (*write_frame_header) (j12_compress_ptr cinfo);
  void (*write_scan_header) (j12_compress_ptr cinfo);
  void (*write_file_trailer) (j12_compress_ptr cinfo);
  void (*write_tables_only) (j12_compress_ptr cinfo);
  /* These routines are exported to allow insertion of extra markers */
  /* Probably only COM and APPn markers should be written this way */
  void (*write_marker_header) (j12_compress_ptr cinfo, int marker,
                               unsigned int datalen);
  void (*write_marker_byte) (j12_compress_ptr cinfo, int val);
};


/* Declarations for decompression modules */

/* Master control module */
struct jpeg12_decomp_master {
  void (*prepare_for_output_pass) (j12_decompress_ptr cinfo);
  void (*finish_output_pass) (j12_decompress_ptr cinfo);

  /* State variables made visible to other modules */
  boolean is_dummy_pass;        /* True during 1st pass for 2-pass quant */

  /* Partial decompression variables */
  JDIMENSION first_iMCU_col;
  JDIMENSION last_iMCU_col;
  JDIMENSION first_MCU_col[MAX_COMPONENTS];
  JDIMENSION last_MCU_col[MAX_COMPONENTS];
  boolean jinit_upsampler_no_alloc;

  /* Last iMCU row that was successfully decoded */
  JDIMENSION last_good_iMCU_row;
};

/* Input control module */
struct jpeg12_input_controller {
  int (*consume_input) (j12_decompress_ptr cinfo);
  void (*reset_input_controller) (j12_decompress_ptr cinfo);
  void (*start_input_pass) (j12_decompress_ptr cinfo);
  void (*finish_input_pass) (j12_decompress_ptr cinfo);

  /* State variables made visible to other modules */
  boolean has_multiple_scans;   /* True if file has multiple scans */
  boolean eoi_reached;          /* True when EOI has been consumed */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg12_d_main_controller {
  void (*start_pass) (j12_decompress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*process_data) (j12_decompress_ptr cinfo, J12SAMPARRAY output_buf,
                        JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail);
};

/* Coefficient buffer control */
struct jpeg12_d_coef_controller {
  void (*start_input_pass) (j12_decompress_ptr cinfo);
  int (*consume_data) (j12_decompress_ptr cinfo);
  void (*start_output_pass) (j12_decompress_ptr cinfo);
  int (*decompress_data) (j12_decompress_ptr cinfo, J12SAMPIMAGE output_buf);
  /* Pointer to array of coefficient virtual arrays, or NULL if none */
  jvirt_barray_ptr *coef_arrays;
};

/* Decompression postprocessing (color quantization buffer control) */
struct jpeg12_d_post_controller {
  void (*start_pass) (j12_decompress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*post_process_data) (j12_decompress_ptr cinfo, J12SAMPIMAGE input_buf,
                             JDIMENSION *in_row_group_ctr,
                             JDIMENSION in_row_groups_avail,
                             J12SAMPARRAY output_buf, JDIMENSION *out_row_ctr,
                             JDIMENSION out_rows_avail);
};

/* Marker reading & parsing */
struct jpeg12_marker_reader {
  void (*reset_marker_reader) (j12_decompress_ptr cinfo);
  /* Read markers until SOS or EOI.
   * Returns same codes as are defined for jpeg12_consume_input:
   * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
   */
  int (*read_markers) (j12_decompress_ptr cinfo);
  /* Read a restart marker --- exported for use by entropy decoder only */
  jpeg12_marker_parser_method read_restart_marker;

  /* State of marker reader --- nominally internal, but applications
   * supplying COM or APPn handlers might like to know the state.
   */
  boolean saw_SOI;              /* found SOI? */
  boolean saw_SOF;              /* found SOF? */
  int next_restart_num;         /* next restart number expected (0-7) */
  unsigned int discarded_bytes; /* # of bytes skipped looking for a marker */
};

/* Entropy decoding */
struct jpeg12_entropy_decoder {
  void (*start_pass) (j12_decompress_ptr cinfo);
  boolean (*decode_mcu) (j12_decompress_ptr cinfo, JBLOCKROW *MCU_data);

  /* This is here to share code between baseline and progressive decoders; */
  /* other modules probably should not use it */
  boolean insufficient_data;    /* set TRUE after emitting warning */
};

/* Inverse DCT (also performs dequantization) */
typedef void (*inverse_DCT_12_method_ptr) (j12_decompress_ptr cinfo,
                                           jpeg_component_info *compptr,
                                           JCOEFPTR coef_block,
                                           J12SAMPARRAY output_buf,
                                           JDIMENSION output_col);

struct jpeg12_inverse_dct {
  void (*start_pass) (j12_decompress_ptr cinfo);
  /* It is useful to allow each component to have a separate IDCT method. */
  inverse_DCT_12_method_ptr inverse_DCT[MAX_COMPONENTS];
};

/* Upsampling (note that upsampler must also call color converter) */
struct jpeg12_upsampler {
  void (*start_pass) (j12_decompress_ptr cinfo);
  void (*upsample) (j12_decompress_ptr cinfo, J12SAMPIMAGE input_buf,
                    JDIMENSION *in_row_group_ctr,
                    JDIMENSION in_row_groups_avail, J12SAMPARRAY output_buf,
                    JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail);

  boolean need_context_rows;    /* TRUE if need rows above & below */
};

/* Colorspace conversion */
struct jpeg12_color_deconverter {
  void (*start_pass) (j12_decompress_ptr cinfo);
  void (*color_convert) (j12_decompress_ptr cinfo, J12SAMPIMAGE input_buf,
                         JDIMENSION input_row, J12SAMPARRAY output_buf,
                         int num_rows);
};

/* Color quantization or color precision reduction */
struct jpeg12_color_quantizer {
  void (*start_pass) (j12_decompress_ptr cinfo, boolean is_pre_scan);
  void (*color_quantize) (j12_decompress_ptr cinfo, J12SAMPARRAY input_buf,
                          J12SAMPARRAY output_buf, int num_rows);
  void (*finish_pass) (j12_decompress_ptr cinfo);
  void (*new_color_map) (j12_decompress_ptr cinfo);
};


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a, b)       ((a) < (b) ? (a) : (b))


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of a JLONG quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS     JLONG shift_temp;
#define RIGHT_SHIFT(x, shft) \
  ((shift_temp = (x)) < 0 ? \
   (shift_temp >> (shft)) | ((~((JLONG)0)) << (32 - (shft))) : \
   (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x, shft)    ((x) >> (shft))
#endif


/* Compression module initialization routines */
EXTERN(void) j12init_compress_master(j12_compress_ptr cinfo);
EXTERN(void) j12init_c_master_control(j12_compress_ptr cinfo,
                                      boolean transcode_only);
EXTERN(void) j12init_c_main_controller(j12_compress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_c_prep_controller(j12_compress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_c_coef_controller(j12_compress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_color_converter(j12_compress_ptr cinfo);
EXTERN(void) j12init_downsampler(j12_compress_ptr cinfo);
EXTERN(void) j12init_forward_dct(j12_compress_ptr cinfo);
EXTERN(void) j12init_huff_encoder(j12_compress_ptr cinfo);
EXTERN(void) j12init_phuff_encoder(j12_compress_ptr cinfo);
EXTERN(void) j12init_arith_encoder(j12_compress_ptr cinfo);
EXTERN(void) j12init_marker_writer(j12_compress_ptr cinfo);
/* Decompression module initialization routines */
EXTERN(void) j12init_master_decompress(j12_decompress_ptr cinfo);
EXTERN(void) j12init_d_main_controller(j12_decompress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_d_coef_controller(j12_decompress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_d_post_controller(j12_decompress_ptr cinfo,
                                       boolean need_full_buffer);
EXTERN(void) j12init_input_controller(j12_decompress_ptr cinfo);
EXTERN(void) j12init_marker_reader(j12_decompress_ptr cinfo);
EXTERN(void) j12init_huff_decoder(j12_decompress_ptr cinfo);
EXTERN(void) j12init_phuff_decoder(j12_decompress_ptr cinfo);
EXTERN(void) j12init_arith_decoder(j12_decompress_ptr cinfo);
EXTERN(void) j12init_inverse_dct(j12_decompress_ptr cinfo);
EXTERN(void) j12init_upsampler(j12_decompress_ptr cinfo);
EXTERN(void) j12init_color_deconverter(j12_decompress_ptr cinfo);
EXTERN(void) j12init_1pass_quantizer(j12_decompress_ptr cinfo);
EXTERN(void) j12init_2pass_quantizer(j12_decompress_ptr cinfo);
EXTERN(void) j12init_merged_upsampler(j12_decompress_ptr cinfo);
/* Memory manager initialization */
EXTERN(void) j12init_memory_mgr(j12_common_ptr cinfo);

#ifndef JPEGINT_H
/* Utility routines in jutils.c */
EXTERN(long) j12div_round_up(long a, long b);
EXTERN(long) j12round_up(long a, long b);
#endif
EXTERN(void) j12copy_sample_rows(J12SAMPARRAY input_array, int source_row,
                                 J12SAMPARRAY output_array, int dest_row,
                                 int num_rows, JDIMENSION num_cols);
#ifndef JPEGINT_H
EXTERN(void) j12copy_block_row(JBLOCKROW input_row, JBLOCKROW output_row,
                               JDIMENSION num_blocks);
EXTERN(void) j12zero_far(void *target, size_t bytestozero);
/* Constant tables in jutils.c */
#if 0                           /* This table is not actually needed in v6a */
extern const int jpeg_zigzag_order[]; /* natural coef order to zigzag order */
#endif
extern const int jpeg12_natural_order[]; /* zigzag coef order to natural
                                            order */

/* Arithmetic coding probability estimation tables in jaricom.c */
extern const JLONG jpeg_aritab[];
#endif

#endif /* JPEG12INT_H */