# Spider Monkey Panel [![version][version-badge]][CHANGELOG] [![Build status](https://ci.appveyor.com/api/projects/status/xs7jno1f96src6qq/branch/master?svg=true)](https://ci.appveyor.com/project/TheQwertiest/foo-spider-monkey-panel/branch/master)

This is a component for the [foobar2000](https://www.foobar2000.org) audio player.

It allows for creation of full-fledged CUI/DUI panels by using nothing more than a Notepad!  
Panels are coded in JavaScript, so you can immediately apply and see the changes you make!

Base functionality includes:
* Graphics functions: drawing of text, external images, lines, rectangles, etc.
* Access to fonts and colours settings from the main preferences of CUI/DUI.
* Execution of main menu and context menu commands.
* Creation of custom buttons and menus.
* Capture of keystrokes/mouse movement/clicks.
* Capture of foobar2000 events with callbacks.
* Processing and changing of file tags.
* Playlists management: create, destroy, sort, change, rename and do everything else that fb2k can do.
* Access to Media Library with ability to sort and filter it's contents.
* Per panel settings storage. 
* Built-in web and filesystem access.
* And more!

## Getting started!

- Use [Installation Guide](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/Installation).
- Take a look at [Documentation and Samples](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/Script-documentation)

## JScript Panel users

The main difference from the excellent `JScript Panel` component by [marc2003](https://github.com/marc2k3) is the underlying JavaScript engine:
- `foo_jscript_panel` uses closed-source `JScript` engine from Internet Explorer, which is abandoned by Microsoft after the development of IE Edge.
- `foo_spider_monkey_panel` uses open-source `SpiderMonkey` engine from Mozilla Firefox, which is actively developed and being improved upon every day!

Main features of `Mozilla SpiderMonkey` engine:
- ECMAScript 2017 conformant JavaScript.
- Potential support of ECMAScript Next once it is released.
- Blazing-fast performance!
- Easily customizable: allows for implementing a lot of features that couldn't be implemented before.

See detailed list of [API Changes][API_CHANGES] for the changes in the JavaScript API.  
Or just use the [Migration Guide](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/JScript-to-SpiderMonkey-migration-guide) to make your scripts compatible.

## Links
[Changelog][CHANGELOG]  
[Detailed list of API Changes][API_CHANGES]  
[Current tasks and plans][TODO]

## Credits
- [marc2003](https://github.com/marc2k3): original foo_jscript_panel, sample scripts and multiple contributions to this project. 
- [T.P. Wang](https://hydrogenaud.io/index.php?action=profile;u=44175): original WSH Panel Mod.  
- [#jsapi IRC channel](https://wiki.mozilla.org/IRC): wouldn't make it through without them, love you guys!  
  Especially huge thanks to sfink and jonco, who spent literally tens of hours helping me!  
- [Respective authors](THIRD_PARTY_NOTICES.md) of the code being used in this project.

[CHANGELOG]: CHANGELOG.md
[TODO]: https://github.com/TheQwertiest/foo_spider_monkey_panel/projects/1
[API_CHANGES]: https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/API-Changes
[version-badge]: https://img.shields.io/badge/version-TBD-blue.svg

