message(STATUS "Fetching FreeType...")

set(BUILD_SHARED_LIBS OFF)
set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)

FetchContent_Declare(FreeType
	GIT_REPOSITORY https://github.com/freetype/freetype
	GIT_TAG e8ebfe988b5f57bfb9a3ecb13c70d9791bce9ecf # 2.12.1
)
FetchContent_MakeAvailable(FreeType)

add_library(dependency_FreeType INTERFACE)
target_include_directories(dependency_FreeType SYSTEM INTERFACE "${freetype_SOURCE_DIR}/include")
target_link_libraries(dependency_FreeType INTERFACE freetype)
