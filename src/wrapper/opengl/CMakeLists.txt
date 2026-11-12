
add_library(AxiomWrapperOpenGL STATIC
    wrapper.cpp
    system.cpp
    context/context.cpp
)


target_include_directories(AxiomWrapperOpenGL PRIVATE
    ${AXIOM_ROOT}
    ${AXIOM_ROOT}/src
    
    ${AXIOM_ROOT}/../glad/include
    ${AXIOM_ROOT}/../glfw/include
    ${AXIOM_ROOT}/../glm
)

if(WIN32)
    target_sources(AxiomWrapperOpenGL PRIVATE
        context/window-windows.cpp
        context/window.cpp
    )

    target_link_libraries(AxiomWrapperOpenGL PRIVATE
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
    target_sources(AxiomWrapperOpenGL PRIVATE
        context/window-unix.cpp
        context/window.cpp
    )

    target_link_libraries(AxiomWrapperOpenGL PRIVATE
        GL
        X11
        dl
        pthread
    )
endif()

target_link_libraries(AxiomWrapperOpenGL PUBLIC
    AxiomCore
    
    glad
    glfw
    glm

    atomic
)

target_compile_options(AxiomWrapperOpenGL PRIVATE
    -g
    -O3
    -fpermissive
    -static
    -Wtemplate-body
    -Wchanges-meaning
)