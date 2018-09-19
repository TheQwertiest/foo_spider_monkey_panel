REQUIREMENTS:
All scripts in the "basic" should work fine on any system.
All scripts in the "complete" folder should work on any system with IE8 or later installed. There are
three exceptions:
"Listenbrainz" requires any version of Windows with IE9 or later installed. It also works under WINE
following the guide on the github wiki.
"Thumbs" can display existing images on any system but requires Windows with at least
IE9 to download new ones. Downloading does not work on WINE.
"Last.fm Lover" doesn't work on WINE. Requires any version of Windows with IE9 or later.
Most scripts in the "complete" folder require the installation of FontAwesome.
https://github.com/FortAwesome/Font-Awesome/blob/fa-4/fonts/fontawesome-webfont.ttf?raw=true

The scripts inside the "jsplaylist-mod" and "js-smooth" folders require the following fonts:
"Guifx v2 Transports.ttf" http://blog.guifx.com/2009/04/02/guifx-v2-transport-font/
"wingdings2.ttf"
"wingdings3.ttf"
If you have Microsoft Office installed, you should have the wingdings fonts. If not, you'll have to
search for them.
"jsplaylist-mod" should work on any system. The 3 "js-smooth" scripts require Windows Vista or later.

USAGE:
Simply copy the text from any .txt file inside the various folders in to a panel's configuration dialog.

The "basic" folder contains some very simple samples that are referred to from "js_doc/Interfaces.js" and
"callbacks.txt" in the "docs" folder.

The "complete" folder contains various samples which are feature complete and have right click options, etc.

IMPORTANT:
Remember that any future component installation will overwrite all files in this directory
so store new files/edits elsewhere.

OUTDATED (No longer works):
The "jsplaylist-mod" folder contains an updated version of Br3tt's excellent JSPlaylist. It was originally
written for WSH panel mod but due to changes in JScript Panel, this mod was created.

The "js-smooth" folder contains Br3tt's "JS Smooth Playlist", "JS Smooth Browser" and
"JS Smooth Playlist Manager" scripts, all updated to be compatible with JScript Panel v2.1.0.1 and
later.
