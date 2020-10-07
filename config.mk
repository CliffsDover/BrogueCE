# Where to look for game data files (found in 'bin'). Must be without trailing slashes!
DATADIR := .

# Include terminal support. Requires ncurses
TERMINAL := NO

# Include graphical support. Requires SDL2 and SDL2_image
GRAPHICS := YES
# Path to sdl2-config script
SDL_CONFIG := sdl2-config

# Select web brogue mode. Requires POSIX system.
WEBBROGUE := NO

# Enable debugging mode. See top of Rogue.h for features
DEBUG := NO

# Configure the executable to run from a macOS .app bundle (only works in graphical mode)
MAC_APP := NO

# Savegames and recordings are either:
#  DESCRIPTIVE - include some details like seed and depth
#  GENERIC - "Saved game.broguesave" and "Recording.broguerec"
FILENAMES := DESCRIPTIVE
