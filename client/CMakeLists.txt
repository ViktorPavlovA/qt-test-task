set(CLIENT_SOURCES
    main.cpp
    DeviceClient.cpp
)

set(CLIENT_HEADERS
    DeviceClient.h
)

add_executable(NPOKalibriClient
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
)

target_link_libraries(NPOKalibriClient
    PRIVATE
        Qt6::Core
        Qt6::Network
)

target_include_directories(NPOKalibriClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS NPOKalibriClient RUNTIME DESTINATION bin)
