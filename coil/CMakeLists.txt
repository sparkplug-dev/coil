set(EXE_NAME "coil")

add_executable(${EXE_NAME} "src/main.cpp")

target_include_directories(${EXE_NAME} 
    PRIVATE "src"
)

target_link_libraries(${EXE_NAME} 
    PRIVATE coil-lib
)
