==============================================================================
Ogg Vorbis playing
==============================================================================

1. Introduction.
2. Commands.
3. Variables.
4. Examples.
5. Binding to controls.
6. Credits.

==============================================================================
1. Introduction.
==============================================================================

Now QuDos has support for playing Ogg Vorbis music files in the directory
"music" under the game directories (e.g. "baseq2"). The music does not stop
when changing levels or episodes, and it supports playlists, sequence play,
and advanced play and seek controls.

The music files in the game directories and pak/pk3 files are merged into the
same list. For example, you can put the Quake II music into "baseq2/music",
and Rogue expansion music into "rogue/music", then when you play Quake II you
will listen the Quake II songs, but when you play Rogue you will listen
Quake II songs and Rogue songs.

The list of music files is loaded when the Ogg Vorbis subsystem starts, so
restarting it will refresh the list.

The list of files has a loopback, it means that if you are playing the last
song and you go to the next one, it will start playing the first one.

==============================================================================
2. Commands.
==============================================================================

- ogg_init
Initialize the Ogg Vorbis subsystem.

- ogg_shutdown
Shut down the Ogg Vorbis subsystem.

- ogg_reinit
Restart the Ogg Vorbis subsystem.

- ogg_play {file | #n | ? | >n | <n}
Play a file, the argument can be one of:
* A file in "music", without the path and ".ogg" extension.
* A # followed by a number, to play the Nth file in the list.
* A ? which indicates to play a random file.
* A > which indicates to advance N positions (defaults to 1).
* A < which indicates to go back N positions (defaults to 1).

- ogg_stop
Stop playing the current file.

- ogg_pause
Pause the current file.

- ogg_resume
Resume the current file.

- ogg_seek {n | >n | <n}
Go to a determinated position of the file in seconds, the argument can be one
of the following:
* n, which indicates to go to the nth position.
* >n, which indicates to advance n positions.
* <n, which indicates to go back n positions.
You can use "ogg_seek >0" and "ogg_seek <0" to get the current position
without changing it.

- ogg_status
Display status (if playing a file, if paused, if stopped, etc.).

==============================================================================
3. Variables.
==============================================================================

- ogg_autoplay {file | #n | ? | >n | <n}
File to play when the Ogg Vorbis subsystem is started (uses ogg_play syntax).
Defaults to "?".

- ogg_enable {0 | 1}
Enable the Ogg Vorbis subsystem when in "1". Defaults to "1".

- ogg_playlist {name}
Use "name" as a list of files instead of listing the contents of "music". Note
that the files must be in "music" and follow ogg_play's syntax for files.
Defaults to "playlist".

- ogg_sequence {next | prev | random | loop | none}
When a file ends, start playing another one, depending on the value:
* next: play the next file.
* prev: play the previous file.
* random: play a random file.
* loop: play the same file again.
* none: stop playing.
Defaults to "next".

- ogg_volume
Volume of the music from 0 to 2. Defaults to "0.7".

==============================================================================
4. Examples.
==============================================================================

Here are some examples of the commands and variables.

Commands:

- Play the first song in the list.
ogg_play #1

- Play the last song in the list.
ogg_play #-1

- Advance three songs in the list.
ogg_play >3

- Go back fifty seconds in the current song.
ogg_seek <50

Variables:

- When Ogg is started, automatically play a random song:
ogg_autoplay ?

- When a song ends, start playing the next one:
ogg_sequence next

==============================================================================
5. Binding to controls.
==============================================================================

If you want to change the current song or run any of the Ogg Vorbis commands
(or variable assignations) with controls (keyboard, mouse, etc.), you can do
it with "bind", for example:

- bind a ogg_play >
Makes "a" move to the next song.

- bind z ogg_play <
Makes "z" move to the previous song.

- bind r ogg_play ?
Makes "r" to play a random song.

- bind x ogg_seek 0
Makes "x" start the song from the begining.

You may also find the MWHEELUP and MWHEELDOWN controls useul for changing
songs.

==============================================================================
6. Credits.
==============================================================================

The S_RawSamplesVol() function, and basic playing was taken as a base. The
rest was written by Alejandro Pulver. Thanks to QuDos for testing and adding
the code to his engine.

==============================================================================
