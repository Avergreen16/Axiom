add_library(AxiomInput STATIC
    input.cpp
)

target_include_directories(AxiomInput
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
        ${AXIOM_ROOT}/external/glad
)

target_link_libraries(AxiomInput
    PUBLIC
        glad
        glfw
)

if(WIN32)
    target_link_libraries(AxiomInput
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
    target_link_libraries(AxiomInput
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomInput PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)