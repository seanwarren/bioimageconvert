rem test mpeg4 mp4 

imgcnv -i movie.tif -o out.%2 -t %1

imgcnv -i movie.tif -o out.fps1.%2 -t %1 -options "fps 1"
imgcnv -i movie.tif -o out.fps2.%2 -t %1 -options "fps 2"
imgcnv -i movie.tif -o out.fps3.%2 -t %1 -options "fps 3"
imgcnv -i movie.tif -o out.fps6.%2 -t %1 -options "fps 6"
imgcnv -i movie.tif -o out.fps15.%2 -t %1 -options "fps 15"
imgcnv -i movie.tif -o out.fps29.97.%2 -t %1 -options "fps 29.97"
imgcnv -i movie.tif -o out.fps30.%2 -t %1 -options "fps 30"

imgcnv -i movie.tif -o out.bps100k.%2 -t %1 -options "bitrate 100000"
imgcnv -i movie.tif -o out.bps1m.%2 -t %1 -options "bitrate 1000000"
imgcnv -i movie.tif -o out.bps10m.%2 -t %1 -options "bitrate 10000000"
imgcnv -i movie.tif -o out.bps100m.%2 -t %1 -options "bitrate 100000000"
imgcnv -i movie.tif -o out.bps500m.%2 -t %1 -options "bitrate 500000000"