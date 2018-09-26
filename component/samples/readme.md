### Requirements
- "basic" scripts:  
         Should work fine on any system.
- "complete" scripts:  
         Should work on any system with IE8 or later installed.  
         Most scripts require the installation of FontAwesome (https://github.com/FortAwesome/Font-Awesome/blob/fa-4/fonts/fontawesome-webfont.ttf?raw=true).  
         There are three exceptions:
   - "Listenbrainz":  
              Requires any version of Windows with IE9 or later installed. It also works under WINE following the guide on the github wiki.
   - "Thumbs":  
              Can display existing images on any system but requires Windows with at least IE9 to download new ones. Downloading does not work on WINE.
   - "Last.fm Lover":  
              Doesn't work on WINE. Requires any version of Windows with IE9 or later.
- "jsplaylist-mod":  
         Should work on any system.  
         Requires the following fonts:
   - "Guifx v2 Transports.ttf" http://blog.guifx.com/2009/04/02/guifx-v2-transport-font/
   - "wingdings2.ttf"
   - "wingdings3.ttf" (If you have Microsoft Office installed, you should have the wingdings fonts.  
                       If not, you'll have to search for them).

### Usage
Simply copy the text from any .txt file inside the various folders in to a panel's configuration dialog.

- "basic":  
         Some very simple samples that are referred to from "js_doc/Interfaces.js" and  
         "Callbacks.js" in the "docs" folder.
- "complete":  
         Feature complete and feature rich samples created by marc2003 (https://github.com/marc2k3/smp_2003).
- "jsplaylist-mod":  
         Port of Br3tt's excellent JSPlaylist (originally written for WSH panel).

### Important
Remember that any future component installation will overwrite all files in this directory
so store new files/edits elsewhere.

OUTDATED (No longer works):
- "js-smooth":  
         Br3tt's "JS Smooth Playlist", "JS Smooth Browser" and "JS Smooth Playlist Manager" scripts.  
         Requires Windows Vista or later.
