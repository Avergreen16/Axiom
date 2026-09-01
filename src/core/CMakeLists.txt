add_library(AxiomCore STATIC
    assets/assets.cpp

    ecs/ecs.cpp

    math/base.cpp
    math/hash.cpp
    math/noise.cpp
    math/octree.cpp 
    math/random.cpp
    math/simd.cpp
    math/3d.cpp
    math/transform2d.cpp
    math/transform3d.cpp

    utilities/utilities.cpp
)

target_include_directories(AxiomCore PUBLIC
    ${AXIOM_ROOT}/src
    ${AXIOM_ROOT}/src/core

    ${AXIOM_ROOT}/../glm
    ${AXIOM_ROOT}/../stb
    ${AXIOM_ROOT}/../xsimd/include
)

target_link_libraries(AxiomCore PRIVATE
    atomic
)

if(WIN32)
    target_link_libraries(AxiomCore PRIVATE
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
elseif(UNIX)
    target_link_libraries(AxiomCore PRIVATE
        GL
        X11
        dl
        pthread
    )
endif()

target_compile_options(AxiomCore PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)