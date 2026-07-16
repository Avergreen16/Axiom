add_library(glad glad.c)

target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/external/glad/include)