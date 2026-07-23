add_library(AxiomUI
    ui.cpp
    ui_system.cpp
    widget_base.cpp
    cursor.cpp
    text.cpp

    screen_widget.cpp
    panel_widget.cpp
    window_widget.cpp
    split_widget.cpp
    row_widget.cpp
    column_widget.cpp
    grid_widget.cpp
    debug_widget.cpp
    button_widget.cpp
    slider_widget.cpp
    tab_widget.cpp
    text_widget.cpp
    spacer_widget.cpp
    drop_widget.cpp
    menu_widget.cpp
    relative_widget.cpp
    input_box_widget.cpp
    text_box_widget.cpp
)

target_include_directories(AxiomUI
    PUBLIC
        ${CMAKE_SOURCE_DIR}/engine
)

target_link_libraries(AxiomUI
    PUBLIC
        AxiomECS
        AxiomUtilities
        AxiomAssets
        AxiomWindow

        glfw

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

target_compile_options(AxiomUI PRIVATE
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
)