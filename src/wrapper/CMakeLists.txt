if(AXIOM_WEBGL)
    add_library(AxiomWrapper STATIC
        webgl/wrapper.cpp
        webgl/input.cpp
        webgl/context.cpp
    )
    
    target_include_directories(AxiomWrapper PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/src
    )
elseif(AXIOM_OPENGL)
    add_library(AxiomWrapper STATIC
        opengl/wrapper.cpp
        opengl/input.cpp
        opengl/context.cpp
    )
    
    target_include_directories(AxiomWrapper PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/src
        
        ${CMAKE_SOURCE_DIR}/external/glad/include
        ${CMAKE_SOURCE_DIR}/external/glfw/include
    )
    
    if(WIN32)
        target_sources(AxiomWrapper PRIVATE
            opengl/window-windows.cpp
        )

        target_link_libraries(AxiomWrapper PRIVATE
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
        target_sources(AxiomWrapper PRIVATE
            opengl/window-unix.cpp
        )

        target_link_libraries(AxiomWrapper PRIVATE
            GL
            X11
            dl
            pthread
        )
    endif()

    target_link_libraries(AxiomWrapper PUBLIC
        AxiomCore
        
        glad
        glfw
        glm

        atomic
    )
endif()

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