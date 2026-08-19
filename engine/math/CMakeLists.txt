add_library(AxiomMath STATIC
    base.cpp
    hash.cpp
    noise.cpp
    octree.cpp 
    random.cpp
    simd.cpp
    3d.cpp
)

target_include_directories(AxiomMath
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
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

        atomic
)

if(WIN32)
    target_link_libraries(AxiomMath
        PRIVATE
            dwmapi
            gdi32
            user32
            shell32
            opengl32
            ws2_32
            wsock32
            mingw32
            mingwex 
    )
elseif(UNIX)
    target_link_libraries(AxiomMath
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomMath PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)