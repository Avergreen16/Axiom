
add_library(AxiomPhysics3D
    collider.cpp
    mesh.cpp
    #physics.cpp
)

target_include_directories(AxiomPhysics3D
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(AxiomPhysics3D
    PUBLIC
        AxiomMath
        AxiomScene
        AxiomECS
        AxiomUtilities

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

target_compile_options(AxiomPhysics3D PRIVATE
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)