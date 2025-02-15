set(LIB_NAME "coil-lib")

add_library(${LIB_NAME} STATIC "src/coil.cpp")

target_include_directories(${LIB_NAME} 
    PUBLIC "include"

    PRIVATE "src"
    PRIVATE ${DBUS_INCLUDE_DIRS}
)

target_link_libraries(${LIB_NAME} 
    PRIVATE ${DBUS_LIBRARIES}
)
