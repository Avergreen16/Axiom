add_executable(Game main.cpp)

# icon.rc

target_include_directories(Game PUBLIC ${CMAKE_SOURCE_DIR}/include)

axiom_set_icon(Game)

set_target_properties(Game PROPERTIES OUTPUT_NAME "Axiom")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine
                 ${CMAKE_CURRENT_BINARY_DIR}/engine)

target_link_libraries(Game 
    PRIVATE
        AxiomWindow
        AxiomMath
        AxiomECS
        AxiomRender
        AxiomUI
        
        glm
)

target_compile_options(Game PRIVATE
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)