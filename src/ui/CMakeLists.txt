set(LIBVKD_ui_SOURCE
    bin.cpp
    console_window.cpp
    node_window.cpp
    timeline.cpp
    performance.cpp
    photo_browser.cpp
    preferences.cpp
    main_ui.cpp
    memory_window.cpp
    render_window.cpp
    inspector.cpp
)

target_sources(vkd PRIVATE ${LIBVKD_ui_SOURCE})