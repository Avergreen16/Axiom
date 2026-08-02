add_library(AxiomScene
    transform2d.cpp
    camera2d.cpp
    transform3d.cpp
    camera3d.cpp
)

target_include_directories(AxiomScene
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(AxiomScene
    PUBLIC
        glad
        glfw
        glm
    
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

target_link_libraries(AxiomScene
    PUBLIC
        AxiomMath
)

target_compile_options(AxiomScene PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)