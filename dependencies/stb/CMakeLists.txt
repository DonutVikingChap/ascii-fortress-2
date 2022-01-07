message(STATUS "Fetching stb...")

FetchContent_Declare(
	stb
	GIT_REPOSITORY https://github.com/nothings/stb
	GIT_TAG af1a5bc352164740c1cc1354942b1c6b72eacb8a # stb_image: 2.27, stb_image_write: 1.16
)

FetchContent_MakeAvailable(stb)

add_library(stb STATIC "stb_image_write.c" "stb_image.c")
target_include_directories(stb PUBLIC "${stb_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(stb PRIVATE
	$<$<C_COMPILER_ID:GNU>:             $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-O3 -flto>  $<$<CONFIG:MinSizeRel>:-Os -flto>   $<$<CONFIG:RelWithDebInfo>:-O3 -g -flto>>
	$<$<C_COMPILER_ID:Clang>:           $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-O3 -flto>  $<$<CONFIG:MinSizeRel>:-Os -flto>   $<$<CONFIG:RelWithDebInfo>:-O3 -g -flto>>
	$<$<C_COMPILER_ID:MSVC>:    /utf-8  $<$<CONFIG:Debug>:/Od>  $<$<CONFIG:Release>:/Ot>        $<$<CONFIG:MinSizeRel>:/Os>         $<$<CONFIG:RelWithDebInfo>:/Ot /Od>>)

add_library(dependency_stb INTERFACE)
target_include_directories(dependency_stb SYSTEM INTERFACE $<TARGET_PROPERTY:stb,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(dependency_stb INTERFACE stb)
