
https://github.com/speedrun-program/amnesia_load_screen_tool

# INSTRUCTIONS

---------------------------------------------------------------------------------------------------
how to make the game skip flashbacks in load screens:
---------------------------------------------------------------------------------------------------
- **CHECK IF THE MODERATORS ALLOW THIS.**
  when this was written, the mods said they wouldn't let people use this feature.
  check if they allow this. If they don't, but you want to use it, tell them you think they should allow it.

- in amnesia_settings.txt, set "skip flashbacks" to "y", "t", or "1".


---------------------------------------------------------------------------------------------------
how to make the game wait through flashbacks in load screens:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "skip flashbacks" to "n", "f", or "0".


---------------------------------------------------------------------------------------------------
how to have menuing load delays for the main menu and for maps in quitouts which you quitout in:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "delay files" to "y", "t", or "1".


---------------------------------------------------------------------------------------------------
how to turn off the menuing load delays for the main menu and for maps in quitouts which you quitout in:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "delay files" to "n", "f", or "0".


---------------------------------------------------------------------------------------------------
how to enable consistent slippery physics:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "enable consistent slippery physics" to "y", "t", or "1".


---------------------------------------------------------------------------------------------------
how to turn off consistent slippery physics:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "enable consistent slippery physics" to "n", "f", or "0".


---------------------------------------------------------------------------------------------------
how to adjust load delays or add or remove maps in maps_and_delays.txt:
---------------------------------------------------------------------------------------------------
- at the start of the line, write the map name followed by a forward slash.
  avoid leading and trailing whitespace around the map name, because that will be seen as part of the name.

- after the forward slash, put the delay in milliseconds.

- example: "12_storage/ 1575"
  this will give the loads from main menu for the Storage map a delay of 1575 milliseconds.


---------------------------------------------------------------------------------------------------
how to add more flashbacks:
---------------------------------------------------------------------------------------------------
- add the sound files to flashback_names.txt.

- the sound files used by each flashback in are listed in the .flash files in \Amnesia The Dark Descent\flashbacks.

- in English, the sound files are in \Amnesia The Dark Descent\lang\eng\voices\flashbacks.
  in Russian, the sound files are in \Amnesia The Dark Descent\lang\rus\voices\flashbacks.
  you can listen to them to check if they're the ones you want to skip.


---------------------------------------------------------------------------------------------------
how to add a delay to the main menu load time:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, adjust the "milliseconds main menu delay" setting.


---------------------------------------------------------------------------------------------------
how to add a delay to quickload load times:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, adjust the "milliseconds quickload delay" setting.

- this only adds a delay to quickloads when there hasn't been any saves since last being in the main menu.
  fortunately, this is the only type of quickload that's used in individual level runs.


---------------------------------------------------------------------------------------------------
how to use the tool with versions of the game it wasn't specifically made for:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "allow unexpected game versions" to "y", "t", or "1".


---------------------------------------------------------------------------------------------------
how to skip checking for updates when injecting Amnesia using the tool:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "check for tool updates" to "n", "f", or "0".


---------------------------------------------------------------------------------------------------
how to use the tool when it isn't, or might not be, the most recent version:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, set "allow not fully updated tool" to "y", "t", or "1".


---------------------------------------------------------------------------------------------------
how to adjust the amount of time that remains in a flashback dialogue line before the next map is entered:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, adjust the "milliseconds remaining before unwait" setting.


---------------------------------------------------------------------------------------------------
how to adjust the amount of time the program tries to check for updates before timing out:
---------------------------------------------------------------------------------------------------
- in amnesia_settings.txt, adjust the "milliseconds update check timeout" setting.
