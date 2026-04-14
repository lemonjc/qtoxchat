set(CLANGD_DB_DIR "${CMAKE_BINARY_DIR}")
configure_file(
    ${CMAKE_SOURCE_DIR}/.clangd.in
    ${CMAKE_SOURCE_DIR}/.clangd
    @ONLY
)