#foo_jscript_panel
foobar2000 component

This is a customisable panel for the [foobar2000](https://www.foobar2000.org) audio player. It is based on [WSH Panel Mod](https://code.google.com/p/foo-wsh-panel-mod/).

All scripts are written in Javascript but the component provides means to do the following:

* Custom drawing of text, images, lines, rectangles, etc.
* Executing main/context menu commands.
* Ability to create custom buttons/menus.
* Callbacks can be used to trigger code based on foobar2000 events . See [callbacks.txt](https://raw.githubusercontent.com/19379/foo-jscript-panel/master/component/docs/Callbacks.txt).
* Read/write file tags.
* Complete manipulation of playlists.
* Media Library display/sorting/filtering
* And much more...

##Update 09/01/2017

Thanks to [TheQwertiest](https://github.com/TheQwertiest), `v.1.2.0` now supports the `Chakra` engine. This means newer ECMAScript5 features are now available. They are documented here:

https://msdn.microsoft.com/library/ff974378.aspx#_ecmascript

This requires you have at least IE9 installed. If you only have IE7/IE8, the standard `JScript` engine will be used.

___

[Requirements & Installation](https://github.com/19379/foo-jscript-panel/wiki/Requirements-&-Installation)

[Changelog](https://github.com/19379/foo-jscript-panel/blob/master/CHANGELOG.md)

[Discussion thread @ hydrogenaud.io](https://hydrogenaud.io/index.php/topic,110499.0.html)
