add_library(AxiomAssets STATIC
    assets.cpp
)

target_include_directories(AxiomAssets
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
        ${AXIOM_ROOT}/external/stb
)

target_link_libraries(AxiomAssets
    PUBLIC
        stb
        glm

        atomic
)

if(WIN32)
    target_link_libraries(AxiomAssets
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
            atomic
    )
elseif(UNIX)
    target_link_libraries(AxiomAssets
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

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