add_library(AxiomWindow
    window.cpp
)

target_include_directories(AxiomWindow
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(AxiomWindow
    PUBLIC
        AxiomPlatform
        AxiomInput

        glad
        glfw
        glm
        assimp
        lua
        asio
        stb
        xsimd

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

target_compile_options(AxiomWindow PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)