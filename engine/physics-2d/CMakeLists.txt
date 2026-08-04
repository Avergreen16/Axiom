
add_library(AxiomPhysics2D
    collider.cpp
    mesh.cpp
    physics.cpp
)

target_include_directories(AxiomPhysics2D
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(AxiomPhysics2D
    PUBLIC
        AxiomMath
        AxiomScene
        AxiomECS

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

target_compile_options(AxiomPhysics2D PRIVATE
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)