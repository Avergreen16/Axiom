add_library(AxiomAssets
    assets.cpp
)

target_link_libraries(AxiomAssets
    PUBLIC
        stb
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

target_link_libraries(AxiomAssets
    PUBLIC
        AxiomUtilities
)

target_compile_options(AxiomAssets PRIVATE
    -O3
    -g
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)