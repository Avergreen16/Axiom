add_library(AxiomRender STATIC
    target.cpp
    func.cpp
    camera2d.cpp
    camera3d.cpp
    system.cpp

    mesh/mesh.cpp

    shadow/shadow.cpp
)

target_include_directories(AxiomRender PRIVATE
    ${AXIOM_ROOT}
    ${AXIOM_ROOT}/src
    
    ${AXIOM_ROOT}/../glad/include
    ${AXIOM_ROOT}/../glfw/include
    ${AXIOM_ROOT}/../glm
)

target_link_libraries(AxiomRender PUBLIC
    AxiomWrapper
    AxiomCore
)

target_compile_options(AxiomRender PRIVATE
    -O3
    -fpermissive
    -static
    -Wtemplate-body
)