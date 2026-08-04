add_library(AxiomRender
    wrapper.cpp
    target.cpp
    mesh.cpp
)

target_link_libraries(AxiomRender
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

target_link_libraries(AxiomRender
    PUBLIC
        AxiomAssets
)

target_compile_options(AxiomRender PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)