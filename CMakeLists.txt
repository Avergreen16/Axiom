set(AXIOM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_library(Axiom INTERFACE)

add_subdirectory(${AXIOM_ROOT}/src/core)
target_link_libraries(Axiom INTERFACE AxiomCore)

if(AXIOM_INCLUDE_PHYSICS2D)
    add_subdirectory(${AXIOM_ROOT}/src/physics-2d)
    target_link_libraries(Axiom INTERFACE AxiomPhysics2D)
endif()

if(AXIOM_INCLUDE_PHYSICS3D)
    add_subdirectory(${AXIOM_ROOT}/src/physics-3d)
    target_link_libraries(Axiom INTERFACE AxiomPhysics3D)
endif()

if(AXIOM_INCLUDE_RENDER)
    add_subdirectory(${AXIOM_ROOT}/src/render)
    target_link_libraries(Axiom INTERFACE AxiomRender)
endif()

if(AXIOM_INCLUDE_UI)
    add_subdirectory(${AXIOM_ROOT}/src/ui)
    target_link_libraries(Axiom INTERFACE AxiomUI)
endif()

target_compile_options(Axiom INTERFACE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)