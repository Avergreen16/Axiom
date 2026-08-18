add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${AXIOM_ROOT}/external/asio/include)

target_compile_definitions(asio INTERFACE ASIO_STANDALONE)