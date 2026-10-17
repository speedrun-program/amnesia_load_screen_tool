
https://github.com/speedrun-program/amnesia_load_screen_tool


# INSTRUCTIONS


+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
amnesia_settings.txt settings
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

---------------------------------------------------------------------------------------------------
skip flashbacks
---------------------------------------------------------------------------------------------------
Controls if flashback dialogue immediately ends in load screens, or if the dialogue keeps playing
and the load screen stay up until the dialogue finishes.

options:
(y / t / 1): Flashback dialogue ends when a load screen appears.
(n / f / 0): Flashback dialogue keeps playing, instead the load screen stays up until the
             dialogue finishes.

---------------------------------------------------------------------------------------------------
delay maps
---------------------------------------------------------------------------------------------------
Controls if map loads have extra load time added when loading them from the main menu, with extra
load time being determined by the number of milliseconds written in the maps_and_delays.txt file.
This setting is meant to help in quitouts runs if the map loads are normally too fast.

options:
(y / t / 1): Maps have extra load time added when loading them from the main menu.
(n / f / 0): Maps have no extra load time added.

---------------------------------------------------------------------------------------------------
enable consistent slippery physics
---------------------------------------------------------------------------------------------------
Controls if slippery physics is set to always be present.

options:
(y / t / 1): Slippery physics becomes always present.
(n / f / 0): Slippery physics probability stays unchanged.

---------------------------------------------------------------------------------------------------
allow unexpected game versions
---------------------------------------------------------------------------------------------------
Controls if the injector will attempt to inject the game if it takes up an unexpected amount
of space in memory.

options:
(y / t / 1): The injector tries to inject the game even if it's an unexpected size in memory.
(n / f / 0): If the game is an unexpected size in memory, the injector doesn't try to inject.

---------------------------------------------------------------------------------------------------
check for tool updates
---------------------------------------------------------------------------------------------------
Controls if the injector checks online, by reading the date in version_date.txt on the GitHub
repository, if it has an update available.

options:
(y / t / 1): The injector checks if it has an update available.
(n / f / 0): The injector skips checking if it has an update available.

---------------------------------------------------------------------------------------------------
allow not fully updated tool
---------------------------------------------------------------------------------------------------
Controls if the injector still tries to inject the game even when it has an update available.

options:
(y / t / 1): The injector still tries to inject the game when there's an update available.
(n / f / 0): The injector only tries to inject the game when there's no update available.

---------------------------------------------------------------------------------------------------
milliseconds main menu delay
---------------------------------------------------------------------------------------------------
How many extra milliseconds to add to main menu loads. This setting is probably obsolete now that
main menu time is ignored.

---------------------------------------------------------------------------------------------------
milliseconds quickload delay
---------------------------------------------------------------------------------------------------
How many extra milliseconds to add to quickload loads. Only applies to quickloads where the
previous load was from the main menu, not to quickloads to quicksaves or map transition saves
or debug menu map load saves. This setting is meant to help with IL runs if the quickloads
are normally too fast.

---------------------------------------------------------------------------------------------------
milliseconds remaining before unwait
---------------------------------------------------------------------------------------------------
The number of milliseconds remaining in a flashback dialogue before the load screen ends.
For example, with the value 435, load screens can end when there's 435 milliseconds or less
remaining in a flashback line.

---------------------------------------------------------------------------------------------------
milliseconds update check timeout
---------------------------------------------------------------------------------------------------
How many milliseconds the injector should try to check online for updates before timing out and
moving on.


+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
flashback_names.txt
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Which flashbacks the game should check for to skip or wait through in load screens.


+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
maps_and_delays.txt
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Which maps should have extra load time added when loading them from the main menu, and how much
extra milliseconds should be added. For example, "16_cistern_entrance/   2500" adds an extra
2500 milliseconds to the load screen when loading the cistern entrance map from the main menu.
