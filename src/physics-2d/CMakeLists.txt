
add_library(AxiomPhysics2D STATIC
    collider.cpp
    mesh.cpp
    physics.cpp
)

target_include_directories(AxiomPhysics2D PUBLIC
    ${AXIOM_ROOT}/src
    ${AXIOM_ROOT}/src/physics-2d
)

target_link_libraries(AxiomPhysics2D PUBLIC
    AxiomCore

    atomic
)

if(WIN32)
    target_link_libraries(AxiomPhysics2D PRIVATE
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
    target_link_libraries(AxiomPhysics2D PRIVATE
        GL
        X11
        dl
        pthread
    )
endif()

target_compile_options(AxiomPhysics2D PRIVATE
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)