add_library(AxiomScene STATIC
    transform2d.cpp
    camera2d.cpp
    transform3d.cpp
    camera3d.cpp
    func.cpp
)

target_include_directories(AxiomScene PUBLIC
    ${AXIOM_ROOT}
    ${AXIOM_ROOT}/src
    ${AXIOM_ROOT}/src/scene
)

target_link_libraries(AxiomScene
    PUBLIC
        glad
        glfw
        glm
    
        atomic
)


if(WIN32)
    target_link_libraries(AxiomScene
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
    target_link_libraries(AxiomScene
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_link_libraries(AxiomScene
    PUBLIC
        AxiomMath
)

target_compile_options(AxiomScene PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)