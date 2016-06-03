find_path(BZIP2_INCLUDE_DIR NAMES bzlib.h)
find_library(BZIP2_LIBRARIES NAMES bz2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(bzip2 REQUIRED_VARS 
                                  BZIP2_INCLUDE_DIR 
                                  BZIP2_LIBRARIES)
