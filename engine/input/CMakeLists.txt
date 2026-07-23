add_library(AxiomInput
    input.cpp
)

target_include_directories(AxiomInput
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(AxiomInput
    PUBLIC
        glad
        glfw

        dwmapi
        gdi32
        user32
        shell32
        opengl32
        ws2_32
        wsock32
        mingw32
        mingwex 
        atomic
)

target_compile_options(AxiomInput PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)