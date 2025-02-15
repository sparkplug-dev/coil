set(DAEMON_NAME "coild")

add_executable(${DAEMON_NAME} "src/main.cpp")

target_include_directories(${DAEMON_NAME} 
    PRIVATE "src"
    PRIVATE ${DBUS_INCLUDE_DIRS}
)

target_link_libraries(${DAEMON_NAME} 
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE ${DBUS_LIBRARIES}
)
