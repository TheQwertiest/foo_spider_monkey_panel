# foo_jscript_panel

This is a component for the [foobar2000](https://www.foobar2000.org)
audio player. It is based on [WSH Panel Mod](https://code.google.com/archive/p/foo-wsh-panel-mod/). 

It allows the creation of customisable panels that can be written
with `Javascript` rather than the `C++` required by the [foobar2000 SDK](https://www.foobar2000.org/SDK).

Under the hood, it uses `Windows Script host` so this means it is
limited to `ECMAScript5` only and won't ever be updated to use
anything more modern. It is possible to use `ActiveX` objects
like `FileSystemObject` and `XMLHttpRequest` to work with the local
file system, access the internet, etc.

Here are just some of the features provided by the component...

* Custom drawing of text, external images, lines, rectangles, etc.
* Use fonts/colours from the main preferences of whichever user interface
  you are using.
* Executing main/context menu commands.
* Ability to create custom buttons/menus.
* Capture keystrokes/mouse movement/clicks.
* Callbacks can be used to trigger code based on foobar2000 events .
  See [callbacks.txt](https://raw.githubusercontent.com/19379/foo-jscript-panel/master/foo_jscript_panel/docs/Callbacks.txt).
* Read/write file tags.
* Complete manipulation of playlists.
* Media Library display/sorting/filtering
* Save settings on a per panel basis. You can also save/load settings
  from .ini files or write your own functions to read and write plain
  text/json files/etc.
* And much more...

## Update 22/09/2017

As of `v1.3.0`, `Internet Explorer 9` or later is now a requirement. Thanks to the
[TheQwertiest](https://github.com/TheQwertiest), newer `ECMAScript5` features
are now available. They are documented here:

https://msdn.microsoft.com/library/ff974378.aspx#_ecmascript

___

[Requirements & Installation](https://github.com/19379/foo-jscript-panel/wiki/Requirements-&-Installation)

[Changelog](https://github.com/19379/foo-jscript-panel/blob/master/CHANGELOG.md)

[Discussion thread @ hydrogenaud.io](https://hydrogenaud.io/index.php/topic,110499.0.html)
