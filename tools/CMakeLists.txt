include_directories(
    ${GLIB2_INCLUDE_DIRS} 
    ${GIO2_INCLUDE_DIRS}
    ${GIOUNIX_INCLUDE_DIRS}
    ${LIBLZMA_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${LIBTAR_INCLUDE_DIRS}
    ${BZIP2_INCLUDE_DIR}
    ${RPM_INCLUDE_DIRS}
    ${POPT_INCLUDE_DIRS}
    ${LIBDPKG_INCLUDE_DIRS}
    ${ELF_INCLUDE_DIRS}
    ${LIBCAP_INCLUDE_DIRS}
    ${MAGIC_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(deb2rpm 
    deb2rpm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/debresolve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/app-pkg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/files.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/fts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/pack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/package.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/reqprov.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../daemon/rpmfc.c
)

target_link_libraries(deb2rpm
    ${GLIB2_LIBRARIES}
    ${GIO2_LIBRARIES}
    ${LIBLZMA_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBTAR_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${RPM_LIBRARIES}
    ${POPT_LIBRARIES}
    ${LIBDPKG_LIBRARIES}
    ${ELF_LIBRARIES}
    ${LIBCAP_LIBRARIES}
    ${MAGIC_LIBRARIES}
)

install(TARGETS deb2rpm RUNTIME DESTINATION bin)
