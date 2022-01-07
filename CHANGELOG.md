# ASCII Fortress 2 Changelog

## Version 1.0.0 | 2017-02-15

-   First version.

## Version 1.1.0 | 2017-02-18

-   Made clients send a disconnect message in their destructor so that the server has a chance to be informed even if they didn't disconnect gracefully.
-   Increased speed of soldier rockets.
-   Reduced sentry fire rate.
-   Fixed spies being able to kill people while dead.
-   Increased flame damage.
-   Increased flamethrower range.
-   Made the "team" command accept "spec" as input.
-   Empty chat messages no longer get sent.
-   Added the ability to send chat messages from the server console.
-   More information is now printed to the server console.
-   Changed the way in-game sounds are loaded and played.
-   Made PlaySound and chat messages use less network bandwidth.
-   Refactored some ugly code.
-   Changed the order in which projectiles are updated to allow for projectiles to be updated on the same frame as they were spawned.
-   Reduced sentry gun server limit from 4294967295 to 65535.
-   Increased brightness of spectator team color
-   Increased engineer sentry build cooldown from 4 seconds to 6 seconds.
-   Increased sentry health to 300
-   Removed mp_player_health cvar and made different classes have different amounts of health. Their health amounts currently match their respective health amounts in TF2.
-   Increased damage of projectiles to compensate for the increased player health.
-   Made it so that you no longer need to choose class after choosing to spectate.
-   Changed the loading of char matrix files (including maps) so that they don't ignore lines with double forward slashes.
-   Made it so that you don't have to add the ".txt" extension to the map name when starting a server.
-   Removed a bunch of spaces that were protruding to the right of map1.
-   Fixed a server crash related to stickybombs.
-   Added a "noclip" command.
-   Made spectators able to move their viewport around the map.
-   Fixed the volume slider not affecting sounds that have already been loaded.
-   Separated explosions from projectiles and thereby fixed a bunch of bugs related to spawning explosions while iterating projectiles.
-   Commands "team" and "class" now list all valid classes.
-   Fixed config files adding an increasing number of spaces after command separators on every game shutdown.
-   Made the server send less messages to unconnected clients.
-   Made the client send less messages to the server before it has connected.
-   Made the health number at the bottom of the screen more visible.
-   Fixed mouse clicks registering even when the game window was out of focus.
-   Added an "sv_hostname" cvar.
-   Added player count, hostname and game version information to ServerInfo messages.
-   Changed team/class select screens to make it more obvious what to type.
-   Made the default player name an empty string, and added a message to the "Join Game" menu asking you to choose a username if you try to connect without changing it.
-   Added a line of sight test to sentry guns so that they don't try to shoot players through walls.
-   Increased heavy move speed.
-   Increased demoman move speed.
-   Made a Windows message box appear when the game displays an error message.
-   Decreased engineer fire rate.
-   Made the client spam unacked reliable messages less fequently.
-   Fixed explosions being able to damage the same player/sentry multiple times.
-   Made stickybombs land on the ground instead of exploding after a certain time.
-   Decreased rocket range.
-   Changed default server timeout to 10 seconds.
-   Changed default client timeout to 10 seconds.
-   Set a maximum length of networked strings.
-   Changed client timeout message to "Server is not responding.".
-   Added support for one-way colliders in maps by using the characters '<', '>', '^' and 'v'.
-   Added respawnroom visualizers.
-   Added resupply cabinets.
-   Updated existing maps with the new available map features.
-   Slightly increased heavy fire rate.
-   Added an "sv_port" cvar.
-   Added a "changelevel" command.
-   Added a "maplist" command.
-   Added a "status" command.
-   Added a "banned_ips" configuration file.
-   Messages are now distributed over multiple packets if there are too many to fit in one.
-   Clients now automatically download the map from the server they are connecting to if they don't already have it installed. This can be disabled with cl_allow_map_download on the client or sv_allow_map_download on the server.
-   Fixed clients sending the wrong number as their ack for reliable messages from the server.
-   Added "download_url", "default_server_ip", "default_port" and "default_map" cvars.
-   Removed test map "map2.txt" from the game download.
-   Renamed "map1.txt" to "ctf_map1.txt".
-   Added 4 new maps: "q3dm1.txt", "ctf_q3dm1.txt", "ctf_turbine.txt" and "ctf_zahn.txt".

## Version 1.1.1 | 2017-02-19

-   Reduced explosion damage to 200.
-   Made explosions deal self damage.
-   Added a server log message for player spawns.
-   Reduced sniper fire rate.
-   Changed default flag return time from 8 seconds to 10 seconds.
-   Fixed flag getting stuck if the flag carrier switched teams.
-   Fixed sentries staying alive when their owner switched to spectator.
-   Increased capture zone to a 3x3 area rather than a single character.
-   Fixed medigun not working.
-   Reduced default sentry health from 300 to 200.
-   Increased sentry build cooldown from 6 seconds to 10 seconds.
-   Slightly reduced scout move speed.
-   Slightly reduced spy move speed.
-   Reduced sentry gun fire rate.
-   Reduced engineer shotgun fire rate.
-   Reduced medic needle gun damage.
-   Made placing a sentry add a cooldown to your primary weapon.
-   Made shooting medic's needle gun add a cooldown to your medigun.
-   Made shooting medic's medigun add a cooldown to your needle gun.

## Version 1.1.2 | 2017-03-07

-   Made clients spam packets less often.
-   Fixed players achieving points for killing themselves.
-   Added the ability to set a server password.
-   Added a quit button to the main menu.
-   Added fullscreen support through the cvar "wnd_fullscreen_mode". The amount of modes available will depend on your graphics card. I personally have 18 modes available, and had the best results using mode 12.
-   Added support for "replace mode" (insert key) in text input fields.
-   Gave sentry guns a 2 second cooldown before they can shoot after being built.
-   Reduced damage of all bullet type projectiles from 75 to 50.
-   Refactored some code and added support for potentially having more than two teams.
-   Slightly increased sentry gun fire rate.
-   Moved client snapshot tick count from packet header to usercmd message.
-   Added cvar "mp_max_team_difference" for setting a limit that moves new players to the team with the least players if the difference between the teams' playercount exceeds a certain value.
-   Gave spies the ability to disguise.
-   Made sentries target undisguised enemy spies.

## Version 1.1.3 | 2017-12-17

-   Updated to SFML 2.4.2
-   Fixed server creating multiple disconnect messages for a single disconnected client.
-   Changed "Password" prompt in the Join Game menu to read "Server password" instead.
-   Changed the way entities are networked from server to client to waste less memory clientside.
-   Changed the way map data is stored and loaded. Map files now require every row to have the same width to work properly.
-   Made the RED/BLU score hud elements overwrite the characters beneath them.
-   Removed the ability to aim in no direction. Players will now aim up by default.
-   When using cl_mouselook 0, the latest aim angles will now be used if no aim button is being pressed.
-   Made the server spam less log messages to the in-game console.
-   Fixed several flag-related bugs on CTF maps.
-   Added the ability to switch classes without dying when standing on a spawnpoint.
-   Added lots of cvars for customizing game balance as the server owner.
-   Increased heavy fire rate to 476.19 RPM
-   Added lots of cvars for customizing which characters are used to draw different entities.
-   Fixed score being applied incorrectly.
-   Made the first client who joins get assigned player id 0 rather than 1.
-   Added default binds to select class using the keypad.
-   Changed default server port to 25566.
-   Lowered default volume to 40.

## Version 1.2.0 | 2018-02-02

-   Added bots!
-   Added a rock the vote system for changing to the next map. Use command "rtv".
-   Added a system that ensures all player names on a server are unique.
-   Fixed player health not resetting when you change class in spawn.
-   Improved the general architecture of the game code.
-   Fixed framerate limiter not actually limiting framerate.
-   Made FPS counter more accurate.
-   Fixed viewport not being properly centered.
-   The scoreboard no longer shows the class of players on the other team.
-   Console output is now in a slightly different format.
-   Removed the ability for the server to tell clients to execute any console command.
-   Empty commands are no longer added to the console's input history.
-   Decreased default soldier rate of fire.
-   Decreased default heavy rate of fire.
-   Decreased default spy move speed.
-   Added a dropdown from RED's flag room in ctf_map1.txt.
-   Changed default stickybomb character to "Q" to distinguish it from corpses.
-   Fixed being able to pick up medkits that were already collected.
-   Changed default "sv_tickrate" to 30.
-   Changed default "cl_cmdrate" to 60.
-   Changed default "fps_max" to 60.
-   Changed default "sv_max_ticks_per_frame" to 8.
-   Added support for comments at the top of map files. Replaced map_credits.txt with comments in the individual map files.
-   Lowered volume of medic's heal beam shoot sound.
-   Fixed player guns sometimes getting drawn at the incorrect angle when using cl_mouselook 0.
-   Player deaths related to joining a team are no longer announced.
-   Added command "serversay" for explicitly sending chat messages as the server.
-   Fixed flag getting returned while being carried if picked up after being dropped.
-   Fixed being able to disguise while carrying the flag.

## Version 1.2.1 | 2018-02-02

-   Bots now change classes on map switch by default.
-   Fixed bots going the wrong way after being interrupted by a fight.
-   Player kill/death/spawn events are no longer logged.
-   Fixed bots going for health and forgetting to capture the objective after a fight.
-   The client now properly prints disconnect messages.

## Version 1.3.0 | 2018-05-18

-   Added payload mode!
-   Added new map pl_map2.txt.
-   Added names over other players' heads that show when you hover your mouse near them. This functionality can be toggled with cl_draw_playernames_friendly/cl_draw_playernames_enemy.
-   Added a round timer. CTF rounds can now result in a stalemate if the time runs out.
-   Added cvars for customizing how likely bots are to select certain classes.
-   Updated sound manager to be more reliable.
-   Changed button layout on the Start Server menu.
-   Fixed mouse aim being slightly off due to an incorrect conversion from screen to char coordinates.
-   Bots now have a chance of choosing to stand still for a moment when dodging enemy fire rather than always moving.
-   Added cvar bot_ai_enable.
-   Added cvar cl_chat_enable.
-   Added cvar cl_char_respawnvis.
-   Added cvars mp_roundtime_ctf and mp_roundtime_payload.
-   Fixed volume of sounds being set incorrectly.
-   Lowered default volume to 25.
-   Fixed typo in comments of ctf_map1.txt.
-   Fixed typos in class select screen.
-   Added additional help to the comments of config.cfg
-   Updated the grammar of some command descriptions.
-   Reduced indentation of text in the console from 3 to 2 spaces.
-   Decreased default chance for bots to select spy.
-   Decreased default sentry health from 200 to 150.
-   Decreased default bot count from 12 to 10.
-   Decreased default pyro move speed.
-   Increased default pyro fire rate.
-   Increased default server and client timeouts from 10 to 20 seconds.
-   Increased default sv_max_ticks_per_frame from 8 to 10.

## Version 2.0.0 | 2022-01-07

### New features

-   Added hats!
-   Added player levels.
-   Added a server browser.
-   Added rocket jumping.
-   Added ammo and reloading.
-   Added new map/gamemode "pong.txt".
-   Added crosshairs.
-   Added hitsounds.
-   Added more settings menus.
-   Added shader options.
-   Added joystick support.
-   Added Linux support.
-   Added many new config files.
-   Added a lot of new console commands and cvars.
-   Added more modding support for custom maps and gamemodes with custom entities.
-   Added a user manual.
-   Added a message that shows how long your respawn time is when you die.
-   Added support for forwarding server commands through in game chat by prefixing messages with '!' or '/'.
-   Added remote console support for server admins.
-   Added round/win limits to allow automatic map rotation.
-   Added automatic team switching between rounds.

### Balance

-   Adjusted a lot of weapon stats.
-   Gave the Soldier a shotgun as a secondary weapon.
-   Made shotgun spread tighter when shot diagonally. This can be disabled with "mp_shotgun_use_legacy_spread 1".
-   Added a respawn time multiplier for the defending team in payload mode when the attackers have pushed the cart a certain distance. By default, the defenders' respawn time is doubled after the cart has been pushed half the way to the end.
-   Decreased default spy move speed.
-   Added a cooldown to the spy's knife.
-   Medics are now awarded scoreboard points for healing teammates.
-   Added a secondary BLU spawn exit to ctf_map1.txt.

### Technical

-   Switched from MSBuild to CMake.
-   Switched from SFML to SDL.
-   Replaced SFML graphics with a custom renderer using OpenGL and GLEW.
-   Replaced SFML audio with SoLoud.
-   Replaced SFML image loading with stb_image and stb_image_write.
-   Replaced SFML font loading with FreeType.
-   Replaced SFML networking with Berkeley sockets.
-   Changed default server port to 25605.
-   Updated connection protocol to support encrypted messages using libsodium.
-   Server passwords are now hashed.
-   Refactored almost all code in one way or another.

### User interface

-   Updated the renderer to only draw one character per tile, like a real terminal.
-   Font size is now automatically adjusted according to the window resoltion, so resizing the window no longer causes stretching.
-   Text inputs now support copy & paste and some more advanced controls.
-   Added sliders, checkboxes and dropdowns.
-   Made the team and class name inputs more forgiving (e.g. "sc" is accepted to mean "scout").
-   Made the HUD team-colored.
-   Made the health value use different colors depending on current health.
-   Made one-way dropdowns a different color from the rest of the map.
-   Added a cart progress indicator to the HUD in payload.
-   Increased brightness of cart track color.
-   Removed the button to open the download page on outdated game version - it now shows a URL instead.
-   Moved the quit button to the top right corner of the main menu.

### Bots

-   Decoupled bots from actual player connections to improve server performance.
-   Made scout bots avoid spies a bit better.
-   Made medic bots a bit less scared of everything.
-   Made demoman bots more likely to move away from the enemy.
-   Made demoman bots smarter about when to detonate their stickies.
-   Made bots spycheck suspicious teammates.
-   Made spy bots less aggressive.
-   Increased the default chance for bots to go for the objective.
-   Made bots not walk on top of the cart.
-   Added a healing cooldown to medic bots to make sure they don't stand still and heal indefinitely.

### General

-   Increased default server tickrate to 60 Hz.
-   Adjusted sound volume.
-   Added date information to log messages in addition to the time.
-   Fixed medics not being able to heal each other.
-   Made spawnpoints cover a 5x5 area where it is safe to switch classes rather than a single point.
-   Fixed medkits not being collected if you stand on top of them when they spawn.
-   Fixed a lot of miscellaneous bugs.
