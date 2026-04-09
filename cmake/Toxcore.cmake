set(BUILD_TOXAV ON CACHE BOOL "Build the tox AV library" FORCE)
set(MUST_BUILD_TOXAV ON CACHE BOOL "Fail the build if toxav cannot be built" FORCE)
set(ENABLE_SHARED ON CACHE BOOL "Build shared (dynamic) libraries" FORCE)
set(ENABLE_STATIC OFF CACHE BOOL "Build static libraries" FORCE)
# Use FLAT_OUTPUT_STRUCTURE to make toxcore output to CMAKE_BINARY_DIR/{bin,lib}
# set(FLAT_OUTPUT_STRUCTURE ON CACHE BOOL "Produce output artifacts in flat structure" FORCE)
set(UNITTEST OFF CACHE BOOL "Disable toxcore unit tests" FORCE)
set(DHT_BOOTSTRAP OFF CACHE BOOL "Disable DHT_bootstrap build" FORCE)
set(BOOTSTRAP_DAEMON OFF CACHE BOOL "Disable tox-bootstrapd build" FORCE)

add_subdirectory(third_party/c-toxcore)

set_target_properties(toxcore_shared PROPERTIES
    # Important on Windows: export all symbols so the import library is generated.
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    # RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    # RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    # RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    # LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib"
    # LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib"
    # ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib"
    # ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib"
)
