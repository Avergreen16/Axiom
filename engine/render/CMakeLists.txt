add_library(AxiomRender STATIC
    wrapper.cpp
    target.cpp
    mesh.cpp
)

target_include_directories(AxiomRender
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
)

target_link_libraries(AxiomRender
    PUBLIC
        glad
        glfw
        glm
    
        atomic
)


if(WIN32)
    target_link_libraries(AxiomRender
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
    target_link_libraries(AxiomRender
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

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