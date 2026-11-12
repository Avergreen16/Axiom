if(AXIOM_WEBGL)
    add_library(AxiomWrapper STATIC
        webgl/wrapper.cpp
        webgl/context/input.cpp
        webgl/context/context.cpp
    )
elseif(AXIOM_OPENGL)

endif()

target_include_directories(AxiomWrapper PUBLIC
    ${AXIOM_ROOT}
    ${AXIOM_ROOT}/src
)


target_link_libraries(AxiomWrapper PUBLIC
    AxiomCore
)

target_compile_options(AxiomWrapper PRIVATE
    -g
    -O3
    -fpermissive
    -static
    -Wtemplate-body
    -Wchanges-meaning
)

#[[
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
]]