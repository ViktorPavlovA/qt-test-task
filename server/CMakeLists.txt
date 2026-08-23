set(SERVER_SOURCES
    main.cpp
    mainwindow/ServerWindow.cpp
    mainwindow/modules/NetworkServer/NetworkServer.cpp
    mainwindow/modules/NetworkServer/modules/ClientSession/ClientSession.cpp
)

set(SERVER_HEADERS
    mainwindow/ServerWindow.h
    mainwindow/modules/NetworkServer/NetworkServer.h
    mainwindow/modules/NetworkServer/modules/ClientSession/ClientSession.h
)

add_executable(NPOKalibriServer
    ${SERVER_SOURCES}
    ${SERVER_HEADERS}
)

target_link_libraries(NPOKalibriServer
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
        Qt6::Gui
)

target_include_directories(NPOKalibriServer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow
        ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow/modules/NetworkServer
        ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow/modules/NetworkServer/modules/ClientSession
)