/*****************************************************************************
 Tag names for metadata

 Author: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Copyright (c) 2010 Vision Research Lab, UCSB <http://vision.ece.ucsb.edu>

 History:
   2010-07-29 17:18:22 - First creation

 Ver : 1
*****************************************************************************/

// image - used to annotate image parameters
DECLARE_STR( IMAGE_DATE_TIME, "date_time" )
DECLARE_STR( IMAGE_FORMAT, "format" )
DECLARE_STR( IMAGE_NAME_TEMPLATE, "image_%d_name" )

// image geometry - describes image parameters
DECLARE_STR( IMAGE_NUM_X, "image_num_x" ) // number pixels in X
DECLARE_STR( IMAGE_NUM_Y, "image_num_y" ) // number pixels in Y
DECLARE_STR( IMAGE_NUM_Z, "image_num_z" ) // number pixels (voxels) in Z
DECLARE_STR( IMAGE_NUM_T, "image_num_t" ) // number of time points
DECLARE_STR( IMAGE_NUM_C, "image_num_c" ) // number of channels
DECLARE_STR( IMAGE_NUM_P, "image_num_p" ) // total number of pages in the image (combines T, Z and possibly C)

DECLARE_STR( IMAGE_NUM_RES_L, "image_num_resolution_levels" )
DECLARE_STR( IMAGE_RES_L_SCALES, "image_resolution_level_scales" )
DECLARE_STR( TILE_NUM_X, "tile_num_x" )
DECLARE_STR( TILE_NUM_Y, "tile_num_y" )
DECLARE_STR( IMAGE_RES_STRUCTURE, "image_resolution_level_structure" )
// there are two possible structures:
DECLARE_STR( IMAGE_RES_STRUCTURE_HIERARCHICAL, "hierarchical" ) // default: structure where tile sizes remain constant
DECLARE_STR( IMAGE_RES_STRUCTURE_FLAT, "flat" ) // structure where tiles get progressively smaller

// image pixels - describes pixel parameters
DECLARE_STR( PIXEL_DEPTH, "image_pixel_depth" ) // bit depth of one pixel in a channel
DECLARE_STR( PIXEL_FORMAT, "image_pixel_format" ) // unsigned, signed, float
DECLARE_STR( IMAGE_MODE, "image_mode" ) // RGB, YUV, LAB...

DECLARE_STR( RAW_ENDIAN, "raw_endian" ) // little, big
DECLARE_STR( IMAGE_DIMENSIONS, "image_dimensions" ) // XYCZT, XYZ, XYC, ...

// resolution - used to provide resolution values for pixels/voxels
DECLARE_STR( PIXEL_RESOLUTION_X, "pixel_resolution_x" )
DECLARE_STR( PIXEL_RESOLUTION_Y, "pixel_resolution_y" )
DECLARE_STR( PIXEL_RESOLUTION_Z, "pixel_resolution_z" )
DECLARE_STR( PIXEL_RESOLUTION_T, "pixel_resolution_t" )

// resolution units - used to provide units for resolution values of pixels/voxels
DECLARE_STR( PIXEL_RESOLUTION_UNIT_X, "pixel_resolution_unit_x" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_Y, "pixel_resolution_unit_y" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_Z, "pixel_resolution_unit_z" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_T, "pixel_resolution_unit_t" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_C, "pixel_resolution_unit_c" )

DECLARE_STR( PIXEL_RESOLUTION_UNIT_MICRONS, "microns" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_SECONDS, "seconds" )
DECLARE_STR( PIXEL_RESOLUTION_UNIT_METERS,  "meters" )

// some typical image descriptions
DECLARE_STR( IMAGE_LABEL, "image/label" )
DECLARE_STR( IMAGE_DESCRIPTION, "image/description" )
DECLARE_STR( IMAGE_NOTES, "image/notes" )
DECLARE_STR( IMAGE_ZOOM, "image/zoom" )
DECLARE_STR( IMAGE_USER, "image/user" )
DECLARE_STR( IMAGE_POSITION, "image/position" )
DECLARE_STR( IMAGE_SIZE, "image/size" )

// objective - information about objective lens
DECLARE_STR( OBJECTIVE_DESCRIPTION, "objective/name" )
DECLARE_STR( OBJECTIVE_MAGNIFICATION, "objective/magnification" )
DECLARE_STR( OBJECTIVE_NUM_APERTURE, "objective/numerical_aperture" )

// channels - used for textual descriptions about image channels
DECLARE_STR( CHANNEL_NAME_0, "channel_0_name" )
DECLARE_STR( CHANNEL_NAME_1, "channel_1_name" )
DECLARE_STR( CHANNEL_NAME_2, "channel_2_name" )
DECLARE_STR( CHANNEL_NAME_3, "channel_3_name" )
DECLARE_STR( CHANNEL_NAME_4, "channel_4_name" )
DECLARE_STR( CHANNEL_NAME_TEMPLATE, "channel_%d_name" )

// display channels - used to map image channels to prefered display colors
// display_channel_red: 3
DECLARE_STR( DISPLAY_CHANNEL_RED,     "display_channel_red" )
DECLARE_STR( DISPLAY_CHANNEL_GREEN,   "display_channel_green" )
DECLARE_STR( DISPLAY_CHANNEL_BLUE,    "display_channel_blue" )
DECLARE_STR( DISPLAY_CHANNEL_YELLOW,  "display_channel_yellow" )
DECLARE_STR( DISPLAY_CHANNEL_MAGENTA, "display_channel_magenta" )
DECLARE_STR( DISPLAY_CHANNEL_CYAN,    "display_channel_cyan" )
DECLARE_STR( DISPLAY_CHANNEL_GRAY,    "display_channel_gray" )

// channel colors - used to map image channels to exact display colors
// channel_color_0: 255,255,0
DECLARE_STR( CHANNEL_COLOR_0, "channel_color_0" )
DECLARE_STR( CHANNEL_COLOR_1, "channel_color_1" )
DECLARE_STR( CHANNEL_COLOR_2, "channel_color_2" )
DECLARE_STR( CHANNEL_COLOR_3, "channel_color_3" )
DECLARE_STR( CHANNEL_COLOR_4, "channel_color_4" )
DECLARE_STR( CHANNEL_COLOR_TEMPLATE, "channel_color_%d" )

// new style channel information
DECLARE_STR( CHANNEL_INFO_TEMPLATE, "channels/channel_%.5d/" )
DECLARE_STR( CHANNEL_INFO_NAME, "name" )
DECLARE_STR( CHANNEL_INFO_DESCRIPTION, "description" )
DECLARE_STR( CHANNEL_INFO_COLOR, "color" ) // channels/channel_00001/color: 255,255,0
DECLARE_STR( CHANNEL_INFO_COLOR_RANGE, "range" )
DECLARE_STR( CHANNEL_INFO_OPACITY, "opacity" )
DECLARE_STR( CHANNEL_INFO_FLUOR, "fluor" )
DECLARE_STR( CHANNEL_INFO_GAMMA, "gamma" )
DECLARE_STR( CHANNEL_INFO_DYE, "dye" )
DECLARE_STR( CHANNEL_INFO_EM_WAVELENGTH, "lsm_emission_wavelength" )
DECLARE_STR( CHANNEL_INFO_EX_WAVELENGTH, "lsm_excitation_wavelength" )
DECLARE_STR( CHANNEL_INFO_PINHOLE_RADIUS, "lsm_pinhole_radius" )
DECLARE_STR( CHANNEL_INFO_OBJECTIVE, "objective" )
DECLARE_STR( CHANNEL_INFO_DETECTOR_GAIN, "detector_gain" )
DECLARE_STR( CHANNEL_INFO_AMPLIFIER_GAIN, "amplifier_gain" )
DECLARE_STR( CHANNEL_INFO_AMPLIFIER_OFFS, "amplifier_offset" )
DECLARE_STR( CHANNEL_INFO_FILTER_NAME, "filter" )
DECLARE_STR( CHANNEL_INFO_POWER, "power" )

// video - information describing the video parameters
DECLARE_STR( VIDEO_FORMAT_NAME, "video_format_name" )
DECLARE_STR( VIDEO_CODEC_NAME, "video_codec_name" )
DECLARE_STR( VIDEO_FRAMES_PER_SECOND, "video_frames_per_second" )

// custom - any other tags in proprietary files should go further prefixed by the custom parent
DECLARE_STR( CUSTOM_TAGS_PREFIX, "custom/" )
DECLARE_STR( RAW_TAGS_PREFIX, "raw/" )

// planes - values per plane of the image
DECLARE_STR( PLANE_DATE_TIME,          "plane_date_time" )
DECLARE_STR( PLANE_DATE_TIME_TEMPLATE, "plane_date_time/%d" )

// stage
DECLARE_STR( STAGE_POSITION_X, "stage_position_x" )
DECLARE_STR( STAGE_POSITION_Y, "stage_position_y" )
DECLARE_STR( STAGE_POSITION_Z, "stage_position_z" )
DECLARE_STR( STAGE_DISTANCE_Z, "stage_distance_z" )

DECLARE_STR( STAGE_POSITION_TEMPLATE_X, "stage_position/%d/x" )
DECLARE_STR( STAGE_POSITION_TEMPLATE_Y, "stage_position/%d/y" )
DECLARE_STR( STAGE_POSITION_TEMPLATE_Z, "stage_position/%d/z" )

DECLARE_STR( CAMERA_SENSOR_X, "camera_sensor_x" )
DECLARE_STR( CAMERA_SENSOR_Y, "camera_sensor_y" )

// Color Profile
DECLARE_STR( ICC_TAGS_PREFIX, "ColorProfile/" )
DECLARE_STR( ICC_TAGS_DEFINITION, "ColorProfile/profile" )
DECLARE_STR( ICC_TAGS_DEFINITION_EMBEDDED, "embedded_icc" )
DECLARE_STR( ICC_TAGS_DEFINITION_SRGB, "sRGB" )

DECLARE_STR( ICC_TAGS_COLORSPACE, "ColorProfile/color_space" )
DECLARE_STR( ICC_TAGS_COLORSPACE_MULTICHANNEL, "multichannel" )
DECLARE_STR( ICC_TAGS_COLORSPACE_RGB,   "RGB" )
DECLARE_STR( ICC_TAGS_COLORSPACE_XYZ,   "XYZ" )
DECLARE_STR( ICC_TAGS_COLORSPACE_LAB,   "Lab" )
DECLARE_STR( ICC_TAGS_COLORSPACE_LUV,   "Luv" )
DECLARE_STR( ICC_TAGS_COLORSPACE_YCBCR, "YCbCr" )
DECLARE_STR( ICC_TAGS_COLORSPACE_YXY,   "Yxy" )
DECLARE_STR( ICC_TAGS_COLORSPACE_GRAY,  "grayscale" )
DECLARE_STR( ICC_TAGS_COLORSPACE_HSV,   "HSV" )
DECLARE_STR( ICC_TAGS_COLORSPACE_HSL,   "HLS" )
DECLARE_STR( ICC_TAGS_COLORSPACE_CMYK,  "CMYK" )
DECLARE_STR( ICC_TAGS_COLORSPACE_CMY,   "CMY" )

// additional image modes
DECLARE_STR( ICC_TAGS_COLORSPACE_MONO,    "monochrome" )
DECLARE_STR( ICC_TAGS_COLORSPACE_INDEXED, "indexed" )
DECLARE_STR( ICC_TAGS_COLORSPACE_RGBA,    "RGBA" )
DECLARE_STR( ICC_TAGS_COLORSPACE_RGBE,    "RGBE" )
DECLARE_STR( ICC_TAGS_COLORSPACE_YUV,     "YUV" )

DECLARE_STR( ICC_TAGS_VERSION,     "ColorProfile/version" )
DECLARE_STR( ICC_TAGS_DESCRIPTION, "ColorProfile/description" )
DECLARE_STR( ICC_TAGS_SIZE,        "ColorProfile/size" )

// Geo
DECLARE_STR( GEO_TAGS_PREFIX, "Geo/" )

// raw tag definitions

DECLARE_STR( RAW_TAGS_OMEXML,    "raw/ome_xml" )
DECLARE_STR( RAW_TYPES_OMEXML,   "string,ome_xml" )

DECLARE_STR( RAW_TAGS_GEOTIFF,   "raw/geotiff" )
DECLARE_STR( RAW_TYPES_GEOTIFF,   "binary,geotiff" )

DECLARE_STR( RAW_TAGS_ICC,       "raw/icc_profile" )
DECLARE_STR( RAW_TYPES_ICC,      "binary,color_profile" )

DECLARE_STR( RAW_TAGS_XMP,       "raw/xmp" )
DECLARE_STR( RAW_TYPES_XMP,      "string,xmp" )

DECLARE_STR( RAW_TAGS_IPTC,      "raw/iptc" )
DECLARE_STR( RAW_TYPES_IPTC,     "binary,iptc" )

DECLARE_STR( RAW_TAGS_PHOTOSHOP,  "raw/photoshop" )
DECLARE_STR( RAW_TYPES_PHOTOSHOP, "binary,photoshop" )

DECLARE_STR( RAW_TAGS_EXIF,      "raw/exif" ) // TIFF image block with EXIF and GPS IFDs
DECLARE_STR( RAW_TYPES_EXIF,     "binary,exif" )

