message(STATUS "Fetching sodium...")

set(SODIUM_DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(SODIUM_MINIMAL OFF CACHE BOOL "" FORCE)
set(SODIUM_ENABLE_BLOCKING_RANDOM OFF CACHE BOOL "" FORCE)

if(NOT BUILD_SHARED_LIBS)
    add_compile_definitions(SODIUM_STATIC=1)
endif()

FetchContent_Declare(sodium
    GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake
    GIT_TAG a606dc79ed346b7c9db6df9ceedd1c3361afcf95 # 1.0.18
)

FetchContent_MakeAvailable(sodium)

target_compile_definitions(sodium PUBLIC SODIUM_STATIC=1)

add_library(dependency_sodium INTERFACE)
target_include_directories(dependency_sodium SYSTEM INTERFACE $<TARGET_PROPERTY:sodium,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_sodium INTERFACE sodium)
