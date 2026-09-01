add_library(AxiomRender STATIC
    wrapper.cpp
    target.cpp
    func.cpp
    camera2d.cpp
    camera3d.cpp
    system.cpp

    mesh/mesh.cpp

    window/input.cpp
)

target_include_directories(AxiomRender PRIVATE
    ${AXIOM_ROOT}
    ${AXIOM_ROOT}/src
    
    ${AXIOM_ROOT}/../glad/include
    ${AXIOM_ROOT}/../glfw/include
    ${AXIOM_ROOT}/../glm
)


if(WIN32)
    target_sources(AxiomRender PRIVATE
        window/window-windows.cpp
        window/window.cpp
    )

    target_link_libraries(AxiomRender PRIVATE
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
    target_sources(AxiomRender PRIVATE
        window/window-unix.cpp
        window/window.cpp
    )

    target_link_libraries(AxiomRender PRIVATE
        GL
        X11
        dl
        pthread
    )
endif()

target_link_libraries(AxiomRender PUBLIC
    AxiomCore
    
    glad
    glfw
    glm

    atomic
)

target_compile_options(AxiomRender PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)