find_path(ELF_INCLUDE_DIR NAMES libelf.h)
find_library(ELF_LIBRARIES NAMES elf)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(elf REQUIRED_VARS 
                                  ELF_INCLUDE_DIR 
                                  ELF_LIBRARIES)
