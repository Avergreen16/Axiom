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
    scroll_widget.cpp
    message_widget.cpp
    render_widget.cpp
    match_widget.cpp
)

target_include_directories(AxiomUI
    PUBLIC
        ${AXIOM_ROOT}
        ${AXIOM_ROOT}/engine
        ${AXIOM_ROOT}/external/glm
)

target_link_libraries(AxiomUI
    PUBLIC
        AxiomECS
        AxiomUtilities
        AxiomAssets
        AxiomWindow
        AxiomRender

        glfw

        atomic
)

if(WIN32)
    target_link_libraries(AxiomUI
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
    target_link_libraries(AxiomUI
        PRIVATE
            GL
            X11
            dl
            pthread
    )
endif()

target_compile_options(AxiomUI PUBLIC
    -g
    -O3
    -fpermissive
    -march=native
    -static
    -Wtemplate-body
    -Wchanges-meaning
    -Wnarrowing
)