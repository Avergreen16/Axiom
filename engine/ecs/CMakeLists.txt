add_library(AxiomECS
    ecs.cpp
)


target_link_libraries(AxiomECS
    PUBLIC
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

target_compile_options(AxiomECS PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)