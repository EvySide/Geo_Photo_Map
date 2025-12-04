# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\GeoPhotoMap_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\GeoPhotoMap_autogen.dir\\ParseCache.txt"
  "GeoPhotoMap_autogen"
  )
endif()
