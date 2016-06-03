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
    ${CMAKE_CURRENT_BINARY_DIR}
)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE} --generate-c-code
                        ${CMAKE_CURRENT_BINARY_DIR}/install-generated --c-namespace Install --interface-prefix org.isoftlinux.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.isoftlinux.Install.xml
                        RESULT_VARIABLE codegen_failed)
if(codegen_failed)
    message(FATAL_ERROR "Error in generating code for install-generated using gdbus-codegen")
endif()

add_executable(isoft-install-daemon 
    main.c
    daemon.c
    rpm-helper.c
    deb-helper.c
    debresolve.c
    app-pkg.c
    files.c
    fts.c
    misc.c
    pack.c
    package.c
    reqprov.c
    rpmfc.c
    extensions.c
    utils.c
    ${CMAKE_CURRENT_BINARY_DIR}/install-generated.c
)

target_link_libraries(isoft-install-daemon
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

install(TARGETS isoft-install-daemon RUNTIME DESTINATION bin)
