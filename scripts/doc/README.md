# Spider Monkey Panel 

All SMP panels are defined with JavaScript, which is:
- Implemented via Mozilla SpiderMonkey engine.
- Conformant with ECMAScript 2017 (with the exception of modules).
- Has built-in ActiveX support (so `Microsoft.XMLHTTP` can be used for network access).
- Has various extensions for managing foobar2000 and interacting with WinAPI.
- Has no built-in web browser-specific objects: e.g. XMLHttpRequest, Web Workers API and etc.

___
#### Table of Contents
- [API description](#api-description)
   - [Handling foobar2000 events](#foobar2000-events)
   - [Controlling foobar2000 and accessing WinAPI](#controlling-foobar2000)
   - [Web access and file system access](#web-and-file-system-access)
- [Samples](#samples)
- [Links](#links)
- [Credits](#credits)
___

## API description<a name="api-description"></a>

### Handling foobar2000 events<a name="foobar2000-events"></a>

Foobar2000 and panel events are translated to callback invocations (see [docs/Callbacks.js](../Callbacks.js) for the full list), for example: 
- `new track being played` event from fb2k will invoke `on_playback_new_track(new_track_handle)` function from user script, if such function was defined.
- `panel's size has changed` > `on_size(width, height)`; 
- `mouse moved` > `on_mouse_move(x, y, mask)`

Note: for performance reasons it's advised to define only those callbacks that you actually need.

### Controlling foobar2000 and accessing WinAPI<a name="controlling-foobar2000"></a>

SMP provides a lot of JavaScript extensions to simplify the work with foobar2000, which are divided in the following namespaces:

- [`fb`](fb.html): functions for controlling foobar2000 and accessing it's data.
- [`gdi`](gdi.html): functions for working with graphics. Most of them are wrappers for Gdi and GdiPlus methods.
- [`plman`](plman.html): functions for managing foobar2000 playlists.
- [`console`](console.html): has only one method [`log`](file:///X:/git/foo_spider_monkey_panel/component/docs/html/console.html#.log), which is used for logging messages to foobar2000 console.
- [`utils`](utils.html): various utility functions.
- [`window`](window.html): functions for working with the current SMP panel and accessing it's properties.

### Web and file-system access<a name="web-and-file-system-access"></a>

Built-in [`ActiveX`](ActiveXObject.html) support further expands the range of available features and allows for easy file-system and web access:
- `Microsoft.XMLHTTP`: this object can be used in place of `XMLHttpRequest` for web access.
- `FileSystemObject`: used for file-system access (see [MSDN documentation](https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/windows-scripting/6kxy1a51(v%3dvs.84))).

Example:
```javascript
var xmlhttp = new ActiveXObject('Microsoft.XMLHTTP');
```

## Samples<a name="samples"></a>

SMP includes some samples which demonstrate it's API usage (see [samples](../../samples) sub-folder of your `foo_spider_monkey_panel` installation):
- [samples/basic](../../samples/basic): basic short examples.
- [samples/complete](../../samples/complete): feature complete and feature rich samples created by [marc2003](https://github.com/marc2k3/smp_2003).

See [samples/readme.md](../../samples/readme.md) for more info.

## Links<a name="links"></a>

[Support thread](https://hydrogenaud.io/index.php/topic,116669.new.html)  
[Changelog][changelog]  
[Detailed list of API Changes][api_changes]  
[Wiki](https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki)

## Credits<a name="credits"></a>
- [marc2003](https://github.com/marc2k3): original [foo_jscript_panel](https://github.com/marc2k3/foo_jscript_panel), [sample scripts](https://github.com/marc2k3/smp_2003) and multiple contributions to this project.
- [T.P. Wang](https://hydrogenaud.io/index.php?action=profile;u=44175): original [WSH Panel Mod](https://code.google.com/archive/p/foo-wsh-panel-mod).
- [#jsapi IRC channel](https://wiki.mozilla.org/IRC): wouldn't make it through without them, love you guys!  
  Especially huge thanks to sfink and jonco, who spent literally tens of hours helping me!
- [Respective authors][third_part_notices] of the code being used in this project.

[changelog]: https://github.com/TheQwertiest/foo_spider_monkey_panel/blob/master/CHANGELOG.md
[third_part_notices]: https://github.com/TheQwertiest/foo_spider_monkey_panel/blob/master/THIRD_PARTY_NOTICES.md
[api_changes]: https://github.com/TheQwertiest/foo_spider_monkey_panel/wiki/API-Changes
