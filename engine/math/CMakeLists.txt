add_library(AxiomMath
    base.cpp
    hash.cpp
    noise.cpp
    octree.cpp 
    random.cpp
    simd.cpp
)


target_link_libraries(AxiomMath
    PUBLIC
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

target_compile_options(AxiomMath PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)