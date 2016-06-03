find_path(MAGIC_INCLUDE_DIR NAMES magic.h)
find_library(MAGIC_LIBRARIES NAMES magic)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(magic REQUIRED_VARS 
                                  MAGIC_INCLUDE_DIR 
                                  MAGIC_LIBRARIES)
