cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project("ASCII Fortress 2"
	VERSION 2.0.0
	DESCRIPTION "Online multiplayer game with ASCII graphics"
	LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "Build dependencies as shared libraries" OFF)
option(AF2_USE_CLANG_TIDY "Use clang-tidy for static analysis" OFF)
option(AF2_USE_CLANG_SANITIZERS "Use AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(NOT BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	add_link_options(-static-libgcc -static-libstdc++)
	if(MINGW)
		add_link_options(-static)
	endif()
endif()

add_subdirectory(dependencies)

add_executable(ascii_fortress_2
	"res/af2.rc"
	"src/console/commands/bot_commands.cpp"
	"src/console/commands/bot_commands.hpp"
	"src/console/commands/entity_commands.cpp"
	"src/console/commands/entity_commands.hpp"
	"src/console/commands/environment_commands.cpp"
	"src/console/commands/environment_commands.hpp"
	"src/console/commands/file_commands.cpp"
	"src/console/commands/file_commands.hpp"
	"src/console/commands/game_client_commands.cpp"
	"src/console/commands/game_client_commands.hpp"
	"src/console/commands/game_client_state_commands.cpp"
	"src/console/commands/game_client_state_commands.hpp"
	"src/console/commands/game_commands.cpp"
	"src/console/commands/game_commands.hpp"
	"src/console/commands/game_server_commands.cpp"
	"src/console/commands/game_server_commands.hpp"
	"src/console/commands/game_server_state_commands.cpp"
	"src/console/commands/game_server_state_commands.hpp"
	"src/console/commands/gui_button_commands.cpp"
	"src/console/commands/gui_button_commands.hpp"
	"src/console/commands/gui_checkbox_commands.cpp"
	"src/console/commands/gui_checkbox_commands.hpp"
	"src/console/commands/gui_commands.cpp"
	"src/console/commands/gui_commands.hpp"
	"src/console/commands/gui_dropdown_commands.cpp"
	"src/console/commands/gui_dropdown_commands.hpp"
	"src/console/commands/gui_slider_commands.cpp"
	"src/console/commands/gui_slider_commands.hpp"
	"src/console/commands/gui_text_input_commands.cpp"
	"src/console/commands/gui_text_input_commands.hpp"
	"src/console/commands/input_manager_commands.cpp"
	"src/console/commands/input_manager_commands.hpp"
	"src/console/commands/inventory_client_commands.cpp"
	"src/console/commands/inventory_client_commands.hpp"
	"src/console/commands/inventory_server_commands.cpp"
	"src/console/commands/inventory_server_commands.hpp"
	"src/console/commands/listen_server_state_commands.cpp"
	"src/console/commands/listen_server_state_commands.hpp"
	"src/console/commands/logger_commands.cpp"
	"src/console/commands/logger_commands.hpp"
	"src/console/commands/logic_commands.cpp"
	"src/console/commands/logic_commands.hpp"
	"src/console/commands/math_commands.cpp"
	"src/console/commands/math_commands.hpp"
	"src/console/commands/meta_client_commands.cpp"
	"src/console/commands/meta_client_commands.hpp"
	"src/console/commands/meta_client_state_commands.cpp"
	"src/console/commands/meta_client_state_commands.hpp"
	"src/console/commands/meta_server_commands.cpp"
	"src/console/commands/meta_server_commands.hpp"
	"src/console/commands/meta_server_state_commands.cpp"
	"src/console/commands/meta_server_state_commands.hpp"
	"src/console/commands/process_commands.cpp"
	"src/console/commands/process_commands.hpp"
	"src/console/commands/remote_console_client_commands.cpp"
	"src/console/commands/remote_console_client_commands.hpp"
	"src/console/commands/remote_console_server_commands.cpp"
	"src/console/commands/remote_console_server_commands.hpp"
	"src/console/commands/sound_manager_commands.cpp"
	"src/console/commands/sound_manager_commands.hpp"
	"src/console/commands/stream_commands.cpp"
	"src/console/commands/stream_commands.hpp"
	"src/console/commands/string_commands.cpp"
	"src/console/commands/string_commands.hpp"
	"src/console/commands/utility_commands.cpp"
	"src/console/commands/utility_commands.hpp"
	"src/console/commands/virtual_machine_commands.cpp"
	"src/console/commands/virtual_machine_commands.hpp"
	"src/console/commands/world_commands.cpp"
	"src/console/commands/world_commands.hpp"
	"src/console/call_frame_handle.cpp"
	"src/console/call_frame_handle.hpp"
	"src/console/command_options.cpp"
	"src/console/command_options.hpp"
	"src/console/command_utilities.hpp"
	"src/console/command.hpp"
	"src/console/con_command.cpp"
	"src/console/con_command.hpp"
	"src/console/convar.cpp"
	"src/console/convar.hpp"
	"src/console/environment.cpp"
	"src/console/environment.hpp"
	"src/console/io_buffer.cpp"
	"src/console/io_buffer.hpp"
	"src/console/process.cpp"
	"src/console/process.hpp"
	"src/console/script.cpp"
	"src/console/script.hpp"
	"src/console/suggestions.cpp"
	"src/console/suggestions.hpp"
	"src/console/virtual_machine.cpp"
	"src/console/virtual_machine.hpp"
	"src/game/client/char_window.cpp"
	"src/game/client/char_window.hpp"
	"src/game/client/game_client.cpp"
	"src/game/client/game_client.hpp"
	"src/game/client/input_manager.cpp"
	"src/game/client/input_manager.hpp"
	"src/game/client/inventory_client.cpp"
	"src/game/client/inventory_client.hpp"
	"src/game/client/remote_console_client.cpp"
	"src/game/client/remote_console_client.hpp"
	"src/game/client/renderer.cpp"
	"src/game/client/renderer.hpp"
	"src/game/client/sound_manager.cpp"
	"src/game/client/sound_manager.hpp"
	"src/game/data/actions.hpp"
	"src/game/data/ammo.hpp"
	"src/game/data/color.cpp"
	"src/game/data/color.hpp"
	"src/game/data/data_type.hpp"
	"src/game/data/direction.hpp"
	"src/game/data/hat.cpp"
	"src/game/data/hat.hpp"
	"src/game/data/health.hpp"
	"src/game/data/inventory.hpp"
	"src/game/data/latency.hpp"
	"src/game/data/player_class.cpp"
	"src/game/data/player_class.hpp"
	"src/game/data/player_id.hpp"
	"src/game/data/projectile_type.cpp"
	"src/game/data/projectile_type.hpp"
	"src/game/data/rectangle.hpp"
	"src/game/data/score.hpp"
	"src/game/data/sound_id.cpp"
	"src/game/data/sound_id.hpp"
	"src/game/data/team.cpp"
	"src/game/data/team.hpp"
	"src/game/data/tick_count.hpp"
	"src/game/data/tickrate.hpp"
	"src/game/data/vector.hpp"
	"src/game/data/weapon.cpp"
	"src/game/data/weapon.hpp"
	"src/game/meta/meta_client_messages.hpp"
	"src/game/meta/meta_client.cpp"
	"src/game/meta/meta_client.hpp"
	"src/game/meta/meta_server_messages.hpp"
	"src/game/meta/meta_server.cpp"
	"src/game/meta/meta_server.hpp"
	"src/game/server/bot.cpp"
	"src/game/server/bot.hpp"
	"src/game/server/entities.hpp"
	"src/game/server/game_server.cpp"
	"src/game/server/game_server.hpp"
	"src/game/server/inventory_server.cpp"
	"src/game/server/inventory_server.hpp"
	"src/game/server/remote_console_server.cpp"
	"src/game/server/remote_console_server.hpp"
	"src/game/server/solid.hpp"
	"src/game/server/world.cpp"
	"src/game/server/world.hpp"
	"src/game/shared/convar_update.hpp"
	"src/game/shared/entities.hpp"
	"src/game/shared/game_client_messages.hpp"
	"src/game/shared/game_server_messages.hpp"
	"src/game/shared/map.cpp"
	"src/game/shared/map.hpp"
	"src/game/shared/resource_info.hpp"
	"src/game/shared/snapshot.cpp"
	"src/game/shared/snapshot.hpp"
	"src/game/shared/special_commands.hpp"
	"src/game/state/game_client_state.cpp"
	"src/game/state/game_client_state.hpp"
	"src/game/state/game_server_state.cpp"
	"src/game/state/game_server_state.hpp"
	"src/game/state/game_state.hpp"
	"src/game/state/listen_server_state.cpp"
	"src/game/state/listen_server_state.hpp"
	"src/game/state/meta_client_state.cpp"
	"src/game/state/meta_client_state.hpp"
	"src/game/state/meta_server_state.cpp"
	"src/game/state/meta_server_state.hpp"
	"src/game/game.cpp"
	"src/game/game.hpp"
	"src/graphics/buffer.cpp"
	"src/graphics/buffer.hpp"
	"src/graphics/error.hpp"
	"src/graphics/font.cpp"
	"src/graphics/font.hpp"
	"src/graphics/framebuffer.cpp"
	"src/graphics/framebuffer.hpp"
	"src/graphics/glsl.hpp"
	"src/graphics/handle.hpp"
	"src/graphics/image.cpp"
	"src/graphics/image.hpp"
	"src/graphics/opengl.hpp"
	"src/graphics/shader.cpp"
	"src/graphics/shader.hpp"
	"src/graphics/texture.cpp"
	"src/graphics/texture.hpp"
	"src/graphics/vertex_array.cpp"
	"src/graphics/vertex_array.hpp"
	"src/gui/button.cpp"
	"src/gui/button.hpp"
	"src/gui/canvas.cpp"
	"src/gui/canvas.hpp"
	"src/gui/checkbox.cpp"
	"src/gui/checkbox.hpp"
	"src/gui/cmd_button.cpp"
	"src/gui/cmd_button.hpp"
	"src/gui/cmd_checkbox.cpp"
	"src/gui/cmd_checkbox.hpp"
	"src/gui/cmd_dropdown.cpp"
	"src/gui/cmd_dropdown.hpp"
	"src/gui/cmd_input.cpp"
	"src/gui/cmd_input.hpp"
	"src/gui/cmd_slider.cpp"
	"src/gui/cmd_slider.hpp"
	"src/gui/console.cpp"
	"src/gui/console.hpp"
	"src/gui/dropdown.cpp"
	"src/gui/dropdown.hpp"
	"src/gui/element.cpp"
	"src/gui/element.hpp"
	"src/gui/layout.hpp"
	"src/gui/menu.cpp"
	"src/gui/menu.hpp"
	"src/gui/slider.cpp"
	"src/gui/slider.hpp"
	"src/gui/text_input.cpp"
	"src/gui/text_input.hpp"
	"src/network/byte_stream.hpp"
	"src/network/config.hpp"
	"src/network/connection.cpp"
	"src/network/connection.hpp"
	"src/network/crypto.hpp"
	"src/network/delta.hpp"
	"src/network/endpoint.cpp"
	"src/network/endpoint.hpp"
	"src/network/message_layout.hpp"
	"src/network/message.hpp"
	"src/network/platform.cpp"
	"src/network/platform.hpp"
	"src/network/socket.cpp"
	"src/network/socket.hpp"
	"src/utilities/algorithm.hpp"
	"src/utilities/arrow_proxy.hpp"
	"src/utilities/countdown.hpp"
	"src/utilities/crc.hpp"
	"src/utilities/file.cpp"
	"src/utilities/file.hpp"
	"src/utilities/integer.hpp"
	"src/utilities/match.hpp"
	"src/utilities/math.hpp"
	"src/utilities/multi_hash.hpp"
	"src/utilities/overloaded.hpp"
	"src/utilities/reference.hpp"
	"src/utilities/registry.hpp"
	"src/utilities/resource.hpp"
	"src/utilities/ring_map.hpp"
	"src/utilities/scope_guard.hpp"
	"src/utilities/span.hpp"
	"src/utilities/string.hpp"
	"src/utilities/tile_matrix.hpp"
	"src/utilities/time.hpp"
	"src/utilities/tuple.hpp"
	"src/utilities/type_list.hpp"
	"src/debug.cpp"
	"src/debug.hpp"
	"src/logger.cpp"
	"src/logger.hpp"
	"src/main.cpp"
)
target_include_directories(ascii_fortress_2 PRIVATE "src")
target_compile_features(ascii_fortress_2 PRIVATE cxx_std_17)
target_compile_options(ascii_fortress_2 PRIVATE
	$<$<OR:$<C_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:GNU>>:        -std=c++17  -Wall -Wextra   -Wpedantic      -Werror                 $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-O3 -flto>  $<$<CONFIG:MinSizeRel>:-Os -flto>   $<$<CONFIG:RelWithDebInfo>:-O3 -g -flto>>
	$<$<OR:$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:Clang>>:    -std=c++17  -Wall -Wextra   -Wpedantic      -Werror                 $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-O3 -flto>  $<$<CONFIG:MinSizeRel>:-Os -flto>   $<$<CONFIG:RelWithDebInfo>:-O3 -g -flto>>
	$<$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>:      /std:c++17  /W3             /permissive-    /WX     /wd4996 /utf-8  $<$<CONFIG:Debug>:/Od>  $<$<CONFIG:Release>:/Ot>        $<$<CONFIG:MinSizeRel>:/Os>         $<$<CONFIG:RelWithDebInfo>:/Ot /Od>>)
target_link_options(ascii_fortress_2 PRIVATE
	$<$<OR:$<C_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:GNU>>:        $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-flto -fuse-linker-plugin>  $<$<CONFIG:MinSizeRel>:-flto -fuse-linker-plugin>   $<$<CONFIG:RelWithDebInfo>:-g -flto -fuse-linker-plugin>>
	$<$<OR:$<C_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:Clang>>:    $<$<CONFIG:Debug>:-g>   $<$<CONFIG:Release>:-flto -fuse-linker-plugin>  $<$<CONFIG:MinSizeRel>:-flto -fuse-linker-plugin>   $<$<CONFIG:RelWithDebInfo>:-g -flto -fuse-linker-plugin>>
	$<$<OR:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>:>)
target_link_libraries(ascii_fortress_2 PRIVATE
	dependency_fmt
	dependency_FreeType
	dependency_GLEW
	dependency_OpenGL
	dependency_SDL2
	dependency_sodium
	dependency_SoLoud
	dependency_stb
	dependency_Threads)

if(MINGW)
	target_link_libraries(ascii_fortress_2 PRIVATE ws2_32)
endif()

if(AF2_USE_CLANG_TIDY)
	find_program(CLANG_TIDY NAMES clang-tidy REQUIRED)
	set_property(TARGET ascii_fortress_2 PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY}")
	if(MSVC)
		set_target_properties(ascii_fortress_2 PROPERTIES
			VS_GLOBAL_RunCodeAnalysis true
			VS_GLOBAL_EnableMicrosoftCodeAnalysis false
			VS_GLOBAL_EnableClangTidyCodeAnalysis true
			VS_GLOBAL_ClangTidyToolExe "${CLANG_TIDY}")
	endif()
endif()

if(AF2_USE_CLANG_SANITIZERS)
	target_compile_options(ascii_fortress_2 PRIVATE -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
	target_link_options(ascii_fortress_2 PRIVATE -g -fsanitize=address,undefined)
endif()

if(BUILD_SHARED_LIBS)
	target_link_libraries(ascii_fortress_2 PRIVATE ${CMAKE_DL_LIBS})
	add_custom_command(TARGET ascii_fortress_2
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:fmt>
			$<TARGET_FILE:GLEW>
			$<TARGET_FILE:SDL2>
			$<TARGET_FILE:sodium>
			$<TARGET_FILE_DIR:ascii_fortress_2>)
endif()

include(GNUInstallDirs)

set_target_properties(ascii_fortress_2 PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
