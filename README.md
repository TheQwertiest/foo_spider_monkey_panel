# Spider Monkey Panel [![version][version-badge]][CHANGELOG] [![Build status](https://ci.appveyor.com/api/projects/status/4fg787ijr73u7mxc/branch/master?svg=true)](https://ci.appveyor.com/project/TheQwertiest/foo-spider-monkey-panel/branch/master)

This is a component for the [foobar2000](https://www.foobar2000.org) audio player.

It allows for creation of full-fledged CUI/DUI panels using JavaScript!  

Base functionality includes:
* Graphics functions: drawing text, external images, lines, rectangles, etc.
* Access fonts and colours settings from the main preferences of CUI/DUI.
* Execution of main menu and context menu commands.
* Creation of custom buttons and menus.
* Capture of keystrokes/mouse movement/clicks.
* Capture of foobar2000 events with callbacks.
* Processing and changing of file tags.
* Playlists management: create, destroy, sort, change, rename and do anything that fb2k can do.
* Access Media Library with ability to sort and filter it's contents.
* Per panel settings storage. 
* Built-in web and filesystem access.
* And more!

## Getting started!

- Use [Installation Guide](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/Installation).
- Take a look at [Documentation and Samples](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/Script-documentation)

## JScript Panel users

The main difference from the excellent `JScript Panel` component by [marc2003](https://github.com/marc2k3) is the underlying JavaScript engine:
- `foo_jscript_panel` uses closed-source `JScript` engine from Internet Explorer, which was abandoned by Microsoft after the development of IE Edge.
- `foo_spider_monkey_panel` uses open-source `SpiderMonkey` engine from Mozilla Firefox, which is being actively developed and improved every day!

Main features of `Mozilla SpiderMonkey` engine:
- ECMAScript 2017 conformant JavaScript.
- Potential support of ECMAScript Next once it has been released.
- Blazing-fast performance!
- Easily customizable: allows to implement lot of features that couldn't be implemented before.

See [wiki][API_CHANGES] for the detailed list of API changes.  
Or just use the [Migration Guide](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/JScript-to-SpiderMonkey-migration-guide) to make your scripts compatible.

## Links
[Support thread](https://hydrogenaud.io/index.php/topic,116669.new.html)  
[Changelog][CHANGELOG]  
[Detailed list of API Changes][API_CHANGES]  
[Current tasks and plans][TODO]  
[Nightly build](https://ci.appveyor.com/api/projects/theqwertiest/foo-spider-monkey-panel/artifacts/_result%2FWin32_Release%2Ffoo_spider_monkey_panel.fb2k-component?branch=master&job=Configuration%3A%20Release)

## Credits
- [marc2003](https://github.com/marc2k3): original [foo_jscript_panel](https://github.com/marc2k3/foo_jscript_panel), [sample scripts](https://github.com/marc2k3/smp_2003) and multiple contributions to this project. 
- [T.P. Wang](https://hydrogenaud.io/index.php?action=profile;u=44175): original [WSH Panel Mod](https://code.google.com/archive/p/foo-wsh-panel-mod).  
- [#jsapi IRC channel](https://wiki.mozilla.org/IRC): wouldn't make it through without them, love you guys!  
  Especially huge thanks to sfink and jonco, who spent literally tens of hours helping me!  
- [Respective authors](THIRD_PARTY_NOTICES.md) of the code being used in this project.

[CHANGELOG]: CHANGELOG.md
[TODO]: https://github.com/TheQwertiest/foo_spider_monkey_panel/projects/1
[API_CHANGES]: https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/API-Changes
[version-badge]: https://img.shields.io/badge/version-1.0.0-blue.svg

