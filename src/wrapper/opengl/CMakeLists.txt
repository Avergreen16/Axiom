
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
