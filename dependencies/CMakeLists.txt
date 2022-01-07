include(FetchContent)

add_subdirectory(fmt)
add_subdirectory(FreeType)
add_subdirectory(OpenGL)
add_subdirectory(GLEW) # Depends on OpenGL.
add_subdirectory(SDL2)
add_subdirectory(sodium)
add_subdirectory(SoLoud) # Depends on SDL2.
add_subdirectory(stb)
add_subdirectory(Threads)
