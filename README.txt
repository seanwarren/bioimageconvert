BioImageConvertor ver: 2.0

Author: Dima V. Fedorov <http://www.dimin.net/>

Arguments: [[-i | -o] FILE_NAME | -t FORMAT_NAME ]

Ex: imgcnv -i 1.jpg -o 2.tif -t TIFF

-brightnesscontrast   - color brightness/contrast adjustment: brightness,contrast, each in range [-100,100], ex: -brightnesscontrast 50,-40


-c                    - additional channels input file name, multiple -c are allowed, in which case multiple channels will be added, -c image must have the same size

-create               - creates a new image with w-width, h-height, z-num z, t-num t, c - channels, d-bits per channel, ex: -create 100,100,1,1,3,8

-deinterlace          - deinterlaces input image with one of the available methods, ex: -deinterlace avg
    odd  - Uses odd lines
    even - Uses even lines
    avg  - Averages lines


-depth                - output depth (in bits) per channel, allowed values now are: 8,16,32,64, ex: -depth 8,D,U
  if followed by comma and [F|D|T|E] allowes to choose LUT method
    F - Linear full range
    D - Linear data range (default)
    T - Linear data range with tolerance ignoring very low values
    E - equalized
    C - type cast
    N - floating point number [0, 1]
    G - Gamma correction, requires setting -gamma
    L - Levels: Min, Max and Gamma correction, requires setting -gamma, -maxv and -minv
  if followed by comma and [U|S|F] the type of output image can be defined
    U - Unsigned integer (with depths: 8,16,32,64) (default)
    S - Signed integer (with depths: 8,16,32,64)
    F - Float (with depths: 32,64,80)
  if followed by comma and [CS|CC] sets channel mode
    CS - channels separate, each channel enhanced separately (default)
    CC - channels combined, channels enhanced together preserving mutual relationships


-display              - creates 3 channel image with preferred channel mapping

-enhancemeta          - Enhances an image beased on preferred settings, currently only CT hounsfield mode is supported, ex: -enhancemeta

-filter               - filters input image, ex: -filter edge
    edge - first derivative
    otsu - b/w masked image
    wndchrmcolor - color quantized hue image

-flip                 - flip the image vertically

-fmt                  - print supported formats

-fmthtml              - print supported formats in HTML

-fmtxml               - print supported formats in XML

-fuse                 - Changes order and number of channels in the output additionally allowing combining channels
Channels separated by comma specifying output channel order (0 means empty channel)
multiple channels can be added using + sign, ex: -fuse 1+4,2+4+5,3

-fuse6                - Produces 3 channel image from up to 6 channels
Channels separated by comma in the following order: Red,Green,Blue,Yellow,Magenta,Cyan,Gray
(0 or empty value means empty channel), ex: -fuse6 1,2,3,4


-fusegrey             - Produces 1 channel image averaging all input channels, uses RGB weights for 3 channel images and equal weights for all others, ex: -fusegrey

-fusemeta             - Produces 3 channel image getting fusion weights from embedded metadata, ex: -fusemeta

-fusemethod           - Defines fusion method, ex: -fusemethod a
  should be followed by comma and [a|m]
    a - Average
    m - Maximum


-fusergb              - Produces 3 channel image from N channels, for each channel an RGB weight should be given
Component contribution are separated by comma and channels are separated by semicolon:
(0 or empty value means no output), ex: -fusergb 100,0,0;0,100,100;0;0,0,100
Here ch1 will go to red, ch2 to cyan, ch3 not rendered and ch4 to blue


-gamma                - sets gamma for histogram conversion: 0.5, 1.0, 2.2, etc, ex: -gamma 2.2


-geometry             - redefines geometry for any incoming image with: z-num z, t-num t and optionally c-num channels, ex: -geometry 5,1 or -geometry 5,1,3

-hounsfield           - enhances CT image using hounsfield scale, ex: -hounsfield 8,U,40,80
  output depth (in bits) per channel, allowed values now are: 8,16,32,64
  followed by comma and [U|S|F] the type of output image can be defined
    U - Unsigned integer (with depths: 8,16,32,64) (default)
    S - Signed integer (with depths: 8,16,32,64)
    F - Float (with depths: 32,64,80)
  followed by comma and window center
  followed by comma and window width
  optionally followed by comma and slope
  followed by comma and intercept, ex: -hounsfield 8,U,40,80,1.0,-1024.0
  if slope and intercept are not set, their values would be red from DICOM metadata, defaulting to 1 and -1024


-i                    - input file name, multiple -i are allowed, but in multiple case each will be interpreted as a 1 page image.

-ihst                 - read image histogram from the file and use for nhancement operations

-il                   - list input file name, containing input file name per line of the text file

-info                 - print image info

-levels               - color levels adjustment: min,max,gamma, ex: -levels 15,200,1.2


-loadomexml           - reads OME-XML from a file and writes if output format is OME-TIFF

-maxv                 - sets max value for histogram conversion, ex: -maxv 240


-meta                 - print image's meta-data

-meta-custom          - print image's custom meta-data fields

-meta-parsed          - print image's parsed meta-data, excluding custom fields

-meta-raw             - print image's raw meta-data in one huge pile

-meta-tag             - prints contents of a requested tag, ex: -tag pixel_resolution

-minv                 - sets min value for histogram conversion, ex: -minv 20


-mirror               - mirror the image horizontally

-mosaic               - compose an image from aligned tiles, ex: -mosaic 512,20,11
  Arguments are defined as SZ,NX,NY where:
    SZ: defines the size of the tile in pixels with width equal to height
    NX - number of tile images in X direction    NY - number of tile images in Y direction

-multi                - creates a multi-paged image if possible (TIFF,AVI), enabled by default

-negative             - returns negative of input image

-no-overlap           - Skips frames that overlap with the previous non-overlapping frame, ex: -no-overlap 5
  argument defines maximum allowed overlap in %, in the example it is 5%


-norm                 - normalize input into 8 bits output

-o                    - output file name

-ohst                 - write image histogram to the file

-ohstxml              - write image histogram to the XML file

-options              - specify encoder specific options, ex: -options "fps 15 bitrate 1000"

Video files AVI, SWF, MPEG, etc. encoder options:
  fps N - specify Frames per Second, where N is a float number, if empty or 0 uses default, ex: -options "fps 29.9"
  bitrate N - specify bitrate in Mb, where N is an integer number, if empty or 0 uses default, ex: -options "bitrate 10000000"

JPEG encoder options:
  quality N - specify encoding quality 0-100, where 100 is best, ex: -options "quality 90"
  progressive no - disables progressive JPEG encoding
  progressive yes - enables progressive JPEG encoding (default)

TIFF encoder options:
  compression N - where N can be: none, packbits, lzw, fax, ex: -options "compression none"
  tiles N - write tiled TIFF where N defined tile size, ex: tiles -options "512"
  pyramid N - writes TIFF pyramid where N is a storage type: subdirs, topdirs, ex: -options "compression lzw tiles 512 pyramid subdirs"



-overlap-sampling     - Defines sampling after overlap detected until no overlap, used to reduce sampling if overlapping, ex: -overlap-sampling 5


-page                 - pages to extract, should be followed by page numbers separated by comma, ex: -page 1,2,5
  page enumeration starts at 1 and ends at number_of_pages
  page number can be a dash where dash will be substituted by a range of values, ex: -page 1,-,5  if dash is not followed by any number, maximum will be used, ex: '-page 1,-' means '-page 1,-,number_of_pages'
  if dash is a first caracter, 1 will be used, ex: '-page -,5' means '-page 1,-,5'

-pixelcounts          - counts pixels above and below a given threshold, requires output file name to store resultant XML file, ex: -pixelcounts 120


-project              - combines by MAX all inout frames into one

-projectmax           - combines by MAX all inout frames into one

-projectmin           - combines by MIN all inout frames into one

-raw                  - reads RAW image with w,h,c,d,p,e,t,interleaved ex: -raw 100,100,3,8,10,0,uint8,1
  w-width, h-height, c - channels, d-bits per channel, p-pages
  e-endianness(0-little,1-big), if in doubt choose 0
  t-pixel type: int8|uint8|int16|uint16|int32|uint32|float|double, if in doubt choose uint8
  interleaved - (0-planar or RRRGGGBBB, 1-interleaved or RGBRGBRGB)

-rawmeta              - print image's raw meta-data in one huge pile

-rearrange3d          - Re-arranges dimensions of a 3D image, ex: -rearrange3d xzy
  should be followed by comma and [xzy|yzx]
    xzy - rearranges XYZ -> XZY
    yzx - rearranges XYZ -> YZX


-reg-points           - Defines quality for image alignment in number of starting points, ex: -reg-points 200
  Suggested range is in between 32 and 512, more points slow down the processing


-remap                - Changes order and number of channels in the output, channel numbers are separated by comma (0 means empty channel), ex: -remap 1,2,3

-res-level            - extract a specified pyramidal level, ex: -res-level 4
    L - is a resolution level, L=0 is native resolution, L=1 is 2X smaller, L=2 is 4X smaller, and so on

-resample             - Is the same as resize, the difference is resample is brute force and resize uses image pyramid for speed

-resize               - should be followed by: width and height of the new image, ex: -resize 640,480
  if one of the numbers is ommited or 0, it will be computed preserving aspect ratio, ex: -resize 640,,NN
  if followed by comma and [NN|BL|BC] allowes to choose interpolation method, ex: -resize 640,480,NN
    NN - Nearest neighbor (default)
    BL - Bilinear
    BC - Bicubic
  if followed by comma [AR|MX|NOUP], the sizes will be limited:
    AR - resize preserving aspect ratio, ex: 640,640,NN,AR
    MX|NOUP - size will be used as maximum bounding box, preserving aspect ratio and not upsampling, ex: 640,640,NN,MX

-resize3d             - performs 3D interpolation on an input image, ex: -resize3d 640,480,16
  if one of the W/H numbers is ommited or 0, it will be computed preserving aspect ratio, ex: -resize3d 640,,16,NN
  if followed by comma and [NN|BL|BC] allowes to choose interpolation method, ex: -resize3d 640,480,16,BC
    NN - Nearest neighbor (default)
    TL - Trilinear
    TC - Tricubic
  if followed by comma AR, the size will be used as maximum bounding box to resize preserving aspect ratio, ex: 640,640,16,BC,AR

-resolution           - redefines resolution for any incoming image with: x,y,z,t where x,y,z are in microns and t in seconds  ex: -resolution 0.012,0.012,1,0

-roi                  - regions of interest, should be followed by: x1,y1,x2,y2 that defines ROI rectangle, ex: -roi 10,10,100,100
  if x1 or y1 are ommited they will be set to 0, ex: -roi ,,100,100 means 0,0,100,100
  if x2 or y2 are ommited they will be set to image size, ex: -roi 10,10,, means 10,10,width-1,height-1
  if more than one region of interest is desired, specify separated by ';', ex: -roi 10,10,100,100;20,20,120,120
  in case of multiple regions, specify a template for output file creation with following variables, ex: -template {output_filename}_{x1}.{y1}.{x2}.{y2}.tif

-rotate               - rotates the image by deg degrees, only accepted valueas now are: 90, -90, 180, guess
guess will extract suggested rotation from EXIF

-sampleframes         - samples for reading every Nth frame (useful for videos), ex: -sampleframes 5

-single               - disables multi-page creation mode

-skip-frames-leading  - skip N initial frames of a sequence, ex: -skip-frames-leading 5

-skip-frames-trailing - skip N final frames of a sequence, ex: -skip-frames-trailing 5

-stretch              - stretch data to it's full range

-superpixels          - Segments image using SLIC superpixel method, takes region size and regularization, ex: -superpixels 16,0.2
    region size is in pixels
    regularization - [0-1], where 0 means shape is least regular

-supported            - prints yes/no if the file can be decoded

-t                    - output format

-template             - Define a template for file names, ex: -template {output_filename}_{n}.tif
  templates specify variables inside {} blocks, available variables vary for different processing

-textureatlas         - Produces a texture atlas 2D image for 3D input images

-texturegrid          - Creates custom texture atlas with: rows,cols ex: -texturegrid 5,7


-threshold            - thresholds the image, ex: -threshold 120,upper
  value is followed by comma and [lower|upper|both] to selet thresholding method
    lower - sets pixels below the threshold to lowest possible value
    upper - sets pixels above or equal to the threshold to highest possible value
    both - sets pixels below the threshold to lowest possible value and above or equal to highest


-tile                 - tile the image and store tiles in the output directory, ex: -tile 256
  argument defines the size of the tiles in pixels
  tiles will be created based on the outrput file name with inserted L, X, Y, where    L - is a resolution level, L=0 is native resolution, L=1 is 2x smaller, and so on    X and Y - are tile indices in X and Y, where the first tile is 0,0, second in X is: 1,0 and so on  ex: '-o my_file.jpg' will produce files: 'my_file_LLL_XXX_YYY.jpg'

  Providing more arguments will instruct extraction of embedded tiles with -tile SZ,XID,YID,L ex: -tile 256,2,4,3
    SZ: defines the size of the tile in pixels
    XID and YID - are tile indices in X and Y, where the first tile is 0,0, second in X is: 1,0 and so on    L - is a resolution level, L=0 is native resolution, L=1 is 2x smaller, and so on

-transform            - transforms input image, ex: -transform fft
    chebyshev - outputs a transformed image in double precision
    fft - outputs a transformed image in double precision
    radon - outputs a transformed image in double precision
    wavelet - outputs a transformed image in double precision

-transform_color      - transforms input image 3 channel image in color space, ex: -transform_color rgb2hsv
    hsv2rgb - converts HSV -> RGB
    rgb2hsv - converts RGB -> HSV

-v                    - prints version

-verbose              - output information about the processing progress, ex: -verbose
  verbose allows argument that defines the amount of info, currently: 1 and 2
  where: 1 is the light info output, 2 is full output

