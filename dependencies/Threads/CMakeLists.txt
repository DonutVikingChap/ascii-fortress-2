set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

get_target_property(INCLUDE_DIRS Threads::Threads INTERFACE_INCLUDE_DIRECTORIES)

add_library(dependency_Threads INTERFACE)
target_include_directories(dependency_Threads SYSTEM INTERFACE ${INCLUDE_DIRS})
target_link_libraries(dependency_Threads INTERFACE Threads::Threads)
