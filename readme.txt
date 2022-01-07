================================================================================
    /\                        -----                                        /--\ 
   /  \              -  -     |                |                          |    |
  /----\   /---  /-- |  |     |--- /--\  |/-- -|-- |/-- /--\ /--- /---        / 
 /      \  \--\ |    |  |     |   |    | |     |   |   |---- \--\ \--\      /   
/        \ ---/  \-- |  |     |    \--/  |      \- |    \--- ---/ ---/    /----|
================================================================================
# ASCII Fortress 2 - User Manual
================================================================================
Version: 2.0.0
Date: 2022-01-07
Authors:
- Donut the Vikingchap: https://steamcommunity.com/id/donutvikingchap/
- See Chapter 11 for third-party credits.

================================================================================
# TABLE OF CONTENTS
================================================================================
1.  Introduction
2.  Gamemodes
  2.1.  Capture the Flag
  2.2.  Payload
3.  Classes
  3.1.  Scout
  3.2.  Soldier
  3.3.  Pyro
  3.4.  Demoman
  3.5.  Heavy
  3.6.  Engineer
  3.7.  Medic
  3.8.  Sniper
  3.9.  Spy
4.  Overview
5.  How to play
  5.1.  Installation
  5.2.  Single player
  5.3.  Multiplayer
  5.4.  In-game
6.  Points, levels and hats
  6.1.  How to earn points
  6.2.  How do points work?
  6.3.  How to equip hats
7.  The Console
  7.1.  What is the console?
  7.2.  Console variables
  7.3.  Special command syntax
  7.4.  Some useful commands
8.  How to customize controls
9.  How to host a server
10. How to create custom maps
  10.1.  About custom maps
  10.2.  Map file structure
  10.3.  Special characters
  10.4.  Custom map script
  10.5.  Syntax highlighting
  10.6.  Custom map resources
11. Credits

================================================================================
# 1. INTRODUCTION
================================================================================
Welcome to ASCII Fortress 2!

This game is a "demake" of TF2 in ASCII console graphics, using a graphical
style similar to early command-line video games, such as "Rogue" from 1980.

The original version of ASCII Fortress 2 was created in 72 hours as an entry to
the TF2Maps.net Winter 2017 72hr jam. Since then, the game has developed into a
much more advanced project with multiple gamemodes, hats, and a full scripting
engine!

Some notable features of ASCII Fortress 2 include:
- Real-time class-based gameplay, complete with guns, in ASCII graphics.
- Online gameplay with up to (theoretically) 65535 players on the same server.
- Offline gameplay with full bot support.
- All 9 TF2 classes with unique gameplay styles.
- 2 included gamemodes: Capture the Flag and Payload.
- 7 included maps.
- Extensive mod support for custom maps and gamemodes with automatic download.
- Console command system for configuration and in-game chat.
- Flexible scripting system for customization using console commands.
- Cross-platform support between Windows and Linux.
- Dedicated server support including headless mode.
- Remote console support for server admins.
- Directional audio.
- Per-server player level, inventory and hats!

================================================================================
# 2. GAMEMODES
================================================================================
--------------------------------------------------------------------------------
## 2.1. Capture the Flag
--------------------------------------------------------------------------------

|
|  !
|

In Capture the Flag, each team starts with a flag in their base. To win, you
must steal the enemy's flag from their base and bring it back to your own in
order to score a point. The first team to reach the score limit wins the round.

--------------------------------------------------------------------------------
## 2.2. Payload
--------------------------------------------------------------------------------

|
|  ..P...............
|

In Payload, the teams consist of a defending team and an attacking team.
The attacking team must push their payload cart to the end of the track to win
the game. The defending team must stop them from doing so within the time limit.

================================================================================
# 3. CLASSES
================================================================================
--------------------------------------------------------------------------------
## 3.1. Scout
--------------------------------------------------------------------------------

|              *            *            *
|  @*          *            *            *
|              *            *            *

Scout's fast movement speed and double capture rate makes him the ideal class
for capturing objectives. His low health makes him vulnerable if stood still,
but with good dodging and well-placed scattergun shots he can hold his own even
in direct combat against stronger classes.

- Health:     125
- Move speed: Fast
- Special:    2x Capture rate
- Weapons:
  - Scattergun:
    - Ammo:   6 shells
    - Damage: 50
    - ROF:    86 RPM

--------------------------------------------------------------------------------
## 3.2. Soldier
--------------------------------------------------------------------------------

|                      XxX
|  @#         o        xXx
|                      XxX

Soldier is the most versatile combat class. Equipped with a powerful rocket
launcher and backup shotgun, his 200 health points make him a formidable
opponent on the battlefield. Soldiers can use their rocket launcher to propel
themselves to the frontline more quickly at the cost of some health.

- Health:     200
- Move speed: Slow
- Special:    Rocket jump
- Weapons:
  - Rocket Launcher:
    - Ammo:   4 rockets
    - Damage: 150
    - ROF:    75 RPM
  - Shotgun:
    - Ammo:   6 shells
    - Damage: 45
    - ROF:    86 RPM

--------------------------------------------------------------------------------
## 3.3. Pyro
--------------------------------------------------------------------------------

|
|  @& f f f f f f
|

Pyro's flamethrower makes him the strongest close-range class. Pyros excel at
ambushing and creating chaos in groups of enemies. The stream of continuous fire
from Pyro's flamethrower can out-damage most opponents when they are caught off-
guard.

- Health:     175
- Move speed: Medium
- Weapons:
  - Flamethrower:
    - Ammo:   200 flames
    - Damage: 40
    - ROF:    600 RPM

--------------------------------------------------------------------------------
## 3.4. Demoman
--------------------------------------------------------------------------------

|                      XxX
|  @x         B        xXx
|                      XxX

The Demoman is a defensive demolitions expert equipped with a remote-detonated
stickybomb launcher. He can lay traps in key locations using his stickybombs and
detonate them on demand using his secondary fire button. He can also use them to
propel himself to the frontline at the cost of a lot of health. Stickybombs can
be destroyed by enemy bullets if left unattended.

- Health:     175
- Move speed: Medium
- Special:    Stickybomb jump
- Weapons:
  - Stickybomb Launcher:
    - Ammo:   8 bombs
    - Damage: 150
    - ROF:    100 RPM
  - Sticky detonator:
    - Ammo:   1 button
    - Damage: 0
    - ROF:    60000 RPM

--------------------------------------------------------------------------------
## 3.5. Heavy
--------------------------------------------------------------------------------

|
|  @H   *   *   *   *   *   *   *   *   *
|

Heavy Weapons Guy is a medium range powerhouse specializing in suppressive fire.
He has the highest health of all the classes and can turn his enemies into
shreds using his devastating minigun. However, Heavy's slow movement speed makes
him an easy target for classes with high burst damage capabilities, such as
snipers and spies.

- Health:     300
- Move speed: Slow
- Weapons:
  - Minigun:
    - Ammo:   200 bullets
    - Damage: 30
    - ROF:    450 RPM

--------------------------------------------------------------------------------
## 3.6. Engineer
--------------------------------------------------------------------------------

|
|  @e         O=    *     *     *     *     *
|

The Engineer is a defensive area denial class capable of building automatic
sentry guns on the battlefield. His sentry guns serve as an excellent
distraction tool and can be placed in key locations of the map for defending
objectives. Engineers are also equipped with a shotgun for self-defense.

- Health:     125
- Move speed: Medium
- Weapons:
  - Shotgun:
    - Ammo:   6 shells
    - Damage: 45
    - ROF:    86 RPM
  - Sentry Gun:
    - Ammo:   infinite bullets
    - Damage: 40
    - ROF:    300 RPM

--------------------------------------------------------------------------------
## 3.7. Medic
--------------------------------------------------------------------------------

|
|  @m  +  +  +  +  +
|

The Medic has the unique ability of healing friendly players using his medi gun.
Medics are instrumental to the sustained fighting capability of their team and
can easily turn the tide of a losing battle. If things go south, the Medic is
equipped with a syringe gun that can be used to ward off chasing enemies.

- Health:     150
- Move speed: Fast
- Weapons:
  - Medi Gun:
    - Ammo:   infinite healing
    - Damage: -50
    - ROF:    360 RPM
  - Syringe Gun:
    - Ammo:   40 syringes
    - Damage: 15
    - ROF:    500 RPM

--------------------------------------------------------------------------------
## 3.8. Sniper
--------------------------------------------------------------------------------

|
|  @_
|

The Sniper is a long-range assassin that can pick off high-value targets using
his slow but accurate sniper rifle. Snipers excel in open spaces but struggle in
close quarters combat. This makes positioning and map knowledge a valuable asset.

- Health:     125
- Move speed: Medium
- Weapons:
  - Sniper Rifle:
    - Ammo:   1 bullet
    - Damage: 150
    - ROF:    30 RPM

--------------------------------------------------------------------------------
## 3.9. Spy
--------------------------------------------------------------------------------

|
|  @s
|

The Spy is a stealthy master of disguise who can infiltrate the enemy base using
his disguise kit to avoid drawing attention. The Spy lacks any ranged attacks,
but can instantly kill enemies by getting close enough and stabbing them with
his butterfly knife. This temporarily breaks his disguise.

- Health:     125
- Move speed: Fast
- Weapons:
  - Knife:
    - Ammo:   1 stab
    - Damage: 500
    - ROF:    30 RPM
  - Disguise Kit:
    - Ammo:   1 disguise
    - Damage: 0
    - ROF:    60 RPM

================================================================================
# 4. OVERVIEW
================================================================================
|
|  #######################################################
|  #RED score: 0            19:21#                ########
|  #BLU score: 0              8. #  + 5.          ########
|  #     7.                      #                ########
|  #                             #  a 6.          ########
|  #                             #                ########
|  #                             #                ########
|  #                9.           #                       #
|  #                 @H          #                       #
|  #                             #########        ########
|  #                                                     #
|  #                                                     #
|  #    ########                                 10.     #
|  #    ########             1.          2.       *      #
|  #    ########              @*          +       *      #
|  #    ########                                  *      #
|  #    ########                                         #
|  #    ########                                         #
|  #       $ {##                                         #
|  #      4. {##                                 /TTTTTTT#
|  #         {##                                /#########
|  #         {##                               /##########
|  #         {##           ########           /###########
|  #         {##           ########           [###########
|  #         {##           ########           [###########
|  #############           ########           [###########
|  #############           ########     11.   [###########
|  #############       3.  ########vvvvvvvvvvv[###########
|  #               Health: 125#Ammo: 6        [###########
|  #######################################################
|  #BOT 4 killed BOT 1 with Sniper Rifle.                #
|  #BOT 3 killed BOT 6 with Shotgun.                     #
|  #BOT 6 killed BOT 9 with Sentry Gun.                  #
|  #BOT 7 killed BOT 2 with Sentry Gun.       13.        #
|  #BOT 6 killed BOT 7 with Sentry Gun.                  #
|  #BOT 6 killed BOT 5 with Sentry Gun.                  #
|  #BOT 10 killed BOT 9 with Rocket Launcher.            #
|  #BOT 10 killed BOT 3 with Rocket Launcher.            #
|  #BOT 5 killed BOT 4 with Minigun.                     #
|  #BOT 7 killed BOT 10 with Shotgun.                    #
|  #=====================================================#
|  #> 12.                                                #
|  #=====================================================#
|  
|  1.  1P Player
|  2.  Crosshair
|  3.  Heads-Up Display (HUD)
|  4.  Resupply cabinet
|  5.  Medkit
|  6.  Ammopack
|  7.  Team score
|  8.  Round timer
|  9.  baka
|  10. Scattergun shots
|  11. One-way dropdown
|  12. Console input
|  13. Console output
|

================================================================================
# 5. HOW TO PLAY
================================================================================
--------------------------------------------------------------------------------
## 5.1. Installation
--------------------------------------------------------------------------------

To install the game from the downloaded zip file, extract the contents to a
folder on your computer where the program has permission to create and write
files. This is necessary to allow your settings to be saved in config files.

For example, we recommend you create a folder named "af2" or "ASCII Fortress 2"
in one of the following locations:

- Your desktop
- Your user's home directory
- A subdirectory of your user's home directory, such as "Games".

Installing into "Program Files" or "Program Files (x86)" is NOT recommended.

To start the game, run the executable named "ascii_fortress_2".
You can create a shortcut to this program and place in a convenient location for
easier access, such as your desktop.

--------------------------------------------------------------------------------
## 5.2. Single player
--------------------------------------------------------------------------------

1. Start the game and click "Start Server".
2. Enter your desired map, username and number of bots.
   The list of installed maps can be viewed using the console command "maplist".
3. If you want to play alone, make sure "Show in server list" is unchecked.
4. Click "Start & Connect".

--------------------------------------------------------------------------------
## 5.3. Multiplayer
--------------------------------------------------------------------------------

1. Start the game and click "Join Game".
2. If you know the server address, enter it and skip to step 5.
3. Click "Server List".
4. Select your favorite server in the server list.
5. Enter your desired username and the server's password (usually empty).
6. Click "Connect".

--------------------------------------------------------------------------------
## 5.4. In-game
--------------------------------------------------------------------------------

- Follow the instructions on the screen to choose a team and class.
- Use W, A, S and D to move.
- Use the mouse cursor to aim (or use the arrow keys).
- Click the left mouse button to fire your primary weapon (or use Space).
- Click the right mouse button to fire your secondary weapon (or use Shift).
- Hold Tab to view the scoreboard.
- Press Y to highlight the console input for server-wide chat.
- Press U to highlight the console input for team-only chat.
- Press Enter to highlight the console input for commands.
- Press Escape to open the settings menu.

================================================================================
# 6. POINTS, LEVELS AND HATS
================================================================================
--------------------------------------------------------------------------------
## 6.1. How to earn points
--------------------------------------------------------------------------------

Points are earned by playing on a server and performing actions that help your
team, such as defeating enemies and capturing the objective. Once you have
earned enough points, you will level up and earn a randomly selected hat which
will be added to your inventory.

--------------------------------------------------------------------------------
## 6.2. How do points work?
--------------------------------------------------------------------------------

Points, levels and hats are stored in your inventory on the server where you
earned them. This means that your level and hats will not transfer between
different servers. You will have a separate inventory on each server, which will
be saved on the server and remembered if you come back to the same server again,
unless the server owner deleted your inventory or lost it in some way.

--------------------------------------------------------------------------------
## 6.3. How to equip hats
--------------------------------------------------------------------------------

You can view the list of hats you have earned on the server you're currently
playing on using the following console command:

  hats

To equip a hat from your inventory, use the following command, where <name> is
the name of the hat you want to equip.

  hat <name>

See Chapter 7 for more information about how to use console commands.

================================================================================
# 7. THE CONSOLE
================================================================================
--------------------------------------------------------------------------------
## 7.1. What is the console?
--------------------------------------------------------------------------------

The console at the bottom of the screen is a very versatile tool that lets you
configure anything and everything about the game. If there is something missing
from the settings menus, you will probably find it in the console. This includes
binding controls; see Chapter 8 for more information about binds.

To execute a command, type it into the console and press Enter. If you get the
command wrong, the console will do its best to help you. Read what it tells you
and follow the advice. One of the most useful features of the console is
auto-complete. Press TAB while typing a command and the console will try to
automatically fill in the rest of the command, or provide suggestions for what
you can type.

Some commands require more input than just the name. These inputs are called
arguments and should follow the command separated by spaces. For example, the
following command adds the numbers 4 and 2 and produces 6 as a result:

  add 4 2

--------------------------------------------------------------------------------
## 7.2. Console variables
--------------------------------------------------------------------------------

Settings that can be changed in the console are called console variables, or
cvars for short. These settings can be viewed or changed using commands.

The value of a console variable can be inspected by using its name as a command.
For example, this command will tell you the width of the game window:

  r_width

To change the value of a console variable, use its name as a command followed by
the desired value as an argument. The following command will change the width of
the game window to 1280:

  r_width 1280

--------------------------------------------------------------------------------
## 7.3. Special command syntax
--------------------------------------------------------------------------------

Console commands can include comments by prefixing the comment with two forward
slashes //. This means everything past the double slashes until the next line
will be ignored by the command interpreter.

If you need to put spaces inside a command argument without it getting split
into several arguments, use {braces} or "quotes".

Examples:

  // The println command prints a line of text to the console output.
  println Text_without_spaces
  println "Text with spaces"
  println {More text with spaces}

Commands can be evaluated inside of other commands so that their result can be
used by a different command. This is done by prefixing the inner command with a
dollar $ symbol. You can also use parentheses () around the command or its
arguments.

For example:

  println_colored red $cmdlist  // Prints the command list in red.
  println_colored red (cmdlist) // Does the same thing as above.
  println_colored red cmdlist() // Does the same thing as above.
  println_colored red add(2 5)  // Prints 7 in red.

--------------------------------------------------------------------------------
## 7.4. Some useful commands
--------------------------------------------------------------------------------

Here are some commands to help you explore and find what you are looking for:

  find <name>  // Search for a command or cvar containing <name>.
  help <name>  // Learn more about the command or cvar named <name>.
  cmdlist      // View the list of all commands (long, but maybe interesting).
  cvarlist     // View the list of all console variables (settings).
  wtf          // Get more information about the latest error.

A full list of console commands and variables with descriptions can be found in
the included file "omake.txt".

================================================================================
# 8. HOW TO CUSTOMIZE CONTROLS
================================================================================
Rebinding the game's controls requires basic knowledge about the console.
See Chapter 7 above to learn more about using console commands.

To reset your controls to default, the game provides the following command:

  default_binds

You can view the current list of binds using the following command:

  bindlist

To add or change a bind, use the command "bind" in the following fashion, where
<input> is the name of the key or button you want to bind, and <command> is the
command which you want that input to execute:

  bind <input> <command>

Examples:

  bind f12 "screenshot"
  bind leftarrow "+aimleft"
  bind a "+left"

To remove a bind, use the command "unbind" like this:

  unbind <input>

Examples:

  unbind f12
  unbind leftarrow
  unbind a

To view which inputs are available, use the following command:

  inputlist

To view which in-game actions exist, use the following command:

  actionlist

Actions start with a + symbol to indicate that they have a corresponding "undo"
action. Undo actions start with a - symbol and are activated when the input is
released. For example, if you bind the A key to +left, then pressing A will
execute +left, and releasing A will execute -left.

================================================================================
# 9. HOW TO HOST A SERVER
================================================================================
The easiest way to host your own server is to use the in-game "Start Server"
menu. Make sure you choose a port that is port-forwarded to your computer in
your router/firewall settings. If people cannot connect to your server, it is
probably because your router is not forwarding traffic on that port to your
computer, or because the traffic is being blocked by a firewall. Please consult
the administrator of your network regarding these issues.

To host a dedicated server, you can run the game in headless mode (i.e. without
graphics) using the launch option -headless. Start the server using the command
start_dedicated.

You can use the config file sv_autoexec.cfg to configure server settings by
writing a sequence of commands which will get executed every time the server is
started. To view all available server settings, use the command "cvarlist" to
search for console variables containing "sv_" (excluding "meta_sv_") like this:

  cvarlist -n sv_ !meta_sv_

You can also view all available server game settings like this:

  cvarlist -n mp_

Here are some particular settings you may find interesting:

  mp_enable_round_time
  mp_limitteams
  mp_roundlimit
  mp_switch_teams_between_rounds
  mp_timelimit
  mp_winlimit
  sv_afk_autokick_time
  sv_allow_resource_download
  sv_bot_count
  sv_cheats
  sv_config_auto_save_interval
  sv_hostname
  sv_map
  sv_map_rotation
  sv_max_clients
  sv_max_connecting
  sv_max_connections_per_ip
  sv_max_players_per_ip
  sv_meta_submit
  sv_motd
  sv_password
  sv_playerlimit
  sv_port
  sv_rcon_enable
  sv_rtv_delay
  sv_rtv_enable
  sv_rtv_needed
  sv_spam_limit
  sv_tickrate
  sv_timeout

The "help" command can be used to get more information about each setting.

You can also find all settings and their descriptions in the "Console Variables"
section of the included file "omake.txt".

================================================================================
# 10. HOW TO CREATE CUSTOM MAPS
================================================================================
--------------------------------------------------------------------------------
## 10.1. About custom maps
--------------------------------------------------------------------------------

ASCII Fortress 2 is designed to make the process of creating your own maps fun
and simple.

Each map is a single text file in the af2/maps directory that contains the map
just as it would look in-game. These map files can be edited using your favorite
text editor. (Notepad++ or Visual Studio Code are recommended.) Try opening one
of the existing map files to see how they look!

Note: If you make any changes to the included maps, you will not be able to join
servers running them because they will detect that your map file is different.
To fix this, you need to delete or re-name the map you made changes to and
re-download the official version of the original map.

You can create your own map files in the maps directory and host them on your
own server. Other players that connect to your server will download the new map
automatically, assuming your server has sv_allow_resource_download set to 1.
Remember to add your map to sv_map_rotation in sv_autoexec.cfg if you want it to
be part of your server's map rotation.

--------------------------------------------------------------------------------
## 10.2. Map file structure
--------------------------------------------------------------------------------

You can only use ASCII characters in your map. Non-standard symbols, like emojis
or non-latin letters, will not display properly in-game.

If your map contains a section starting with [DATA] and ending with
[END_DATA], then everything outside of that section will not be part of the
map. This allows you to put comments at the top of the file such as map name
and author information, just like in the official map files.

Using a [DATA]/[END_DATA] section also lets you specify special characters for
defining gameplay logic. The gamemode of your map is determined by which special
characters you use in the map.

--------------------------------------------------------------------------------
## 10.3. Special characters
--------------------------------------------------------------------------------

Directional one-way dropdowns can be created using the following characters:

- Left: <
- Right: >
- Up: ^
- Down: v

You can choose what characters you want to use in the map file to represent the
entities that control the in-game gamemode logic. These are specified using a
special tag in brackets followed by a space and then your desired character.
Here is a list of all the special tags and the corresponding characters that the
official maps use for them as examples:

[SPAWN_RED] 1
[SPAWN_BLU] 2
[RESUPPLY] 3
[MEDKIT] 4
[AMMOPACK] 5
[FLAG_RED] 6
[FLAG_BLU] 7
[SPAWNVIS_RED] 8
[SPAWNVIS_BLU] 9
[TRACK_RED] ,
[TRACK_BLU] .
[CART_RED] Q
[CART_BLU] P

You can change these to other characters if, for example, you want to use digits
as normal walls in your map. The characters you decide to use for entities will
not affect how the entities look in-game.

--------------------------------------------------------------------------------
## 10.4. Custom map script
--------------------------------------------------------------------------------

If you want your map to include custom gameplay logic, this can be achieved
using a map script. This is a section in your map file starting with [SCRIPT]
and ending with [END_SCRIPT] that contains console commands which the server
will execute when your map is loaded. See ctf_debug.txt for an example of how
this can look. For an extreme example, see pong.txt and its accompanying
gamemode file cfg/gamemodes/pong.cfg.

Map scripts may contain functions with special names, known as callbacks, which
will be executed by the server when certain events occur. Here is the full list
of callbacks that the server will execute, including their arguments:

+---------------------------+---------------+---------------+---------------+
| Callback                  | Argument 1    | Argument 2    | Argument 3    |
+---------------------------+---------------+---------------+---------------+
| on_map_start              |               |               |               |
| on_map_end                |               |               |               |
| on_round_start            |               |               |               |
| on_round_reset            |               |               |               |
| on_round_won              | team_id       |               |               |
| on_stalemate              |               |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_player_join            | player_id     |               |               |
| on_player_leave           | player_id     |               |               |
| on_team_select            | player_id     |               |               |
| on_player_spawn           | player_id     |               |               |
| on_resupply               | player_id     |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_kill_player            | player_id     | killer_id     |               |
| on_kill_sentry            | sentry_id     | killer_id     |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_player_create          | player_id     |               |               |
| on_projectile_create      | projectile_id |               |               |
| on_explosion_create       | explosion_id  |               |               |
| on_sentry_create          | sentry_id     |               |               |
| on_medkit_create          | medkit_id     |               |               |
| on_ammopack_create        | ammopack_id   |               |               |
| on_ent_create             | ent_id        |               |               |
| on_flag_create            | flag_id       |               |               |
| on_cart_create            | cart_id       |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_medkit_spawn           | medkit_id     |               |               |
| on_ammopack_spawn         | ammopack_id   |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_pickup_medkit          | medkit_id     | player_id     |               |
| on_pickup_ammopack        | ammopack_id   | player_id     |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_pickup_flag            | flag_id       | player_id     |               |
| on_drop_flag              | flag_id       | player_id     |               |
| on_capture_flag           | flag_id       | player_id     |               |
| on_return_flag            | flag_id       |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_push_cart              | cart_id       |               |               |
| on_capture_cart           | cart_id       |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_collide_ent_player     | ent_id        | player_id     |               |
| on_collide_ent_projectile | ent_id        | projectile_id |               |
| on_collide_ent_explosion  | ent_id        | explosion_id  |               |
| on_collide_ent_sentry     | ent_id        | sentry_id     |               |
| on_collide_ent_medkit     | ent_id        | medkit_id     |               |
| on_collide_ent_ammopack   | ent_id        | ammopack_id   |               |
| on_collide_ent_ent        | ent_a_id      | ent_b_id      |               |
| on_collide_ent_flag       | ent_id        | flag_id       |               |
| on_collide_ent_cart       | ent_id        | cart_id       |               |
| on_collide_ent_world      | ent_id        | normal_x      | normal_y      |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_ent_step               | ent_id        | x             | y             |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_pre_tick               | delta_time    |               |               |
| on_post_tick              | delta_time    |               |               |
| ~~~~~~~~~~~~~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ | ~~~~~~~~~~~~~ |
| on_server_receive_command | sender_ip     | command       | arguments...  |
| on_chat                   | sender_ip     | message       |               |
| on_team_chat              | sender_ip     | team_id       | message       |
+---------------------------+---------------+---------------+---------------+

--------------------------------------------------------------------------------
## 10.5. Syntax highlighting
--------------------------------------------------------------------------------

If you would like to set up VS Code for syntax highlighting of the scripting
language used by ASCII Fortress 2, you can use the included TextMate grammar in
"af2script.tmLanguage.json" which can be found in the game's "cfg" folder.
For more information, please consult the relevant VS Code documentation on how
to create a custom language extension.

--------------------------------------------------------------------------------
## 10.6. Custom map resources
--------------------------------------------------------------------------------

If maps require custom resources (such as extra script files) to be downloaded
alongside the map, these can be listed in a section starting with [RESOURCES]
and ending with [END_RESOURCES]. Each resource is a relative file path from the
game's data directory. For example:

[RESOURCES]
cfg/gamemodes/my_gamemode_script.cfg
screens/my_screen.txt
[END_RESOURCES]

See pong.txt for an example.

================================================================================
# 11. CREDITS
================================================================================
This game includes community-contributed maps. Author information can be found
inside the individual map files.

Portions of this software are copyright (C) 2021 The FreeType
Project (www.freetype.org). All rights reserved.
