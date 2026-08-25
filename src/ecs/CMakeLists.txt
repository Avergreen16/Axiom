add_library(AxiomECS STATIC
    ecs.cpp
)

target_include_directories(AxiomECS PUBLIC
    ${AXIOM_ROOT}/src
    ${AXIOM_ROOT}/src/ecs
)

target_link_libraries(AxiomECS
    PUBLIC
        AxiomUtilities

        atomic
)

if(WIN32)
    target_link_libraries(AxiomECS
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
    target_link_libraries(AxiomECS
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomECS PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -Wchanges-meaning
)