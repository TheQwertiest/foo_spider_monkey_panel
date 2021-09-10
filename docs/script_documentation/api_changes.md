---
title: API changes
parent: Script documentation
nav_order: 3
---

# API changes
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

For the latest detailed documentation use `docs/html/index.html` inside your `foo_spider_monkey_panel` installation (e.g. `foobar2000/user-components/foo_spider_monkey_panel/docs`).

Legend:
- API marked with **\[Experimental]** is subject to change and might not be stable or fully functional.  
- API marked with **\[Deprecated]** will be removed in future versions and thus should not be used.

___

## v1.6.0

### Added
- Added a boolean `IsInternal` field to `action` argument (that is passed to `on_drag_*` callbacks).
  This field holds `true` if the drag-n-drop session was started by `fb.DoDragDrop()` inside the same panel.
- Added an ability to generate main menu items dynamically and handle it via the following API:
  - `fb.RegisterMainMenuCommand(id, name, description)`: registers a main menu item that will be displayed under `main menu`>`File`>`Spider Monkey Panel`>`Script commands`>`{Current panel name}`>`{name}`.
  - `fb.UnregisterMainMenuCommand(id, name)`: unregisters the item from `fb.RegisterMainMenuCommand()`.
  Execution of registered command will trigger the new `on_main_menu_dynamic(id)` callback.
  
  Note: SMP uses a combination of panel name and command id to identify and bind the command. Hence all the corresponding binds will fail if the id or the panel name
  is changed. This also means that collision WILL occur if there are two panels with the same name.
- Expanded playlist *undo* API: `Undo`/`Redo` menu items only apply to active playlist, the following methods are introduced to rectify that:
  - `plman.Undo(playlistIndex)`.
  - `plman.Redo(playlistIndex)`.
  - `plman.IsUndoAvailable(playlistIndex)`.
  - `plman.IsRedoAvailable(playlistIndex)`.

### Changed
- `fb.DoDragDrop()` is now executed asynchronously. Which means that it might exit before `on_drag_drop` callback is triggered when dropping data on the same panel as the one that had a call to `fb.DoDragDrop()`.
- `window.NotifyOthers()` is now executed asynchronously, hence the data passed via `info` argument
  must NOT be used or modified in the source panel after invoking this method.
- `on_main_menu()` callback is marked as **\[Deprecated]**. Use dynamically generated main menu items and corresponding `on_main_menu_dynamic()` callback instead.

## v1.5.1

### Added
- Added `plman.GetPlaylistLockName(playlist_index)` method. Returns the name of the lock. It can be used to check whether the lock is owned by `foo_spider_monkey_panel` and
  thus can be modified with `plman.SetPlaylistLockedActions()`.

## v1.5.0

### Added
- Added `plman.GetPlaylistLockedActions(playlist_index)` method. Returns an array of strings, which represent actions that can't be performed on the specidified playlist.  
  These actions may be one or multiple of the following: `AddItems`, `RemoveItems`, `ReorderItems`, `ReplaceItems`, `RenamePlaylist`, `RemovePlaylist`, `ExecuteDefaultAction`.
- Added `plman.SetPlaylistLockedActions(playlist_index, locked_actions)` method. Sets actions that can't be performed on the specified playlist.  
  Accepts an array of strings that may contain the same items as actions described above for `plman.GetPlaylistLockedActions()`.
- Added `utils.GetPackageInfo(package_id)` method. Returns `null`, if the package is not found, or, if found, an object of the following format:
```javascript
{
    Version, // Package version
    Directories: {
        Root, // Root directory of the package
        Script, // Directory inside package folder that contains assets
        Assets, // Directory inside package folder that contains 
        Storage // Persistent and unique directory inside foobar2000 profile folder that can be used to store runtime data (e.g. cache)
    }
}
```
- Added `fb.Restart()` method. Restarts the player.

### Changed
- Addeded an optional `use_exact` argument to `GdiGraphics.CalcTextWidth()`. `false` by default. If set to `true` will use a more accurate width calculation algorithm in certain cases. See ([#140](https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/140)) for more info on these cases.
- `utils.GetPackagePath()` is marked as **\[Deprecated]**. Use `utils.GetPackageInfo()` instead.
- `plman.IsPlaylistLocked()` is marked as **\[Deprecated]**. Use `plman.GetPlaylistLockedActions()` instead.

## v1.4.1

### Changed 
- **\[BREAKING]** Changed casing of `window.JsMemoryStats` fields for consistency with the rest of API: 
  - `memory_usage` to `MemoryUsage`.
  - `total_memory_usage` to `TotalMemoryUsage`.
  - `total_memory_limit` to `TotalMemoryLimit`.

## v1.4.0

### Added
- Added iterator protocol support to `FbMetadbHandleList`: see <https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Iteration_protocols> for more info.
- Added iterator protocol support to enumerable `ActiveXObject` objects.
- Added `type` argument to `FbUiSelectionHolder.SetSelection()`. Where `type` can be one of the following values:
  - 0: undefined (no item), default value.
  - 1: active_playlist_selection.
  - 2: caller_active_playlist.
  - 3: playlist_manager.
  - 4: now_playing.
  - 5: keyboard_shortcut_list.
  - 6: media_library_viewer.
- Added `fb.Version` property: returns foobar2000 version as a string, e.g. `1.4.1`.
- Added `utils.DetectCharset(path)` method. Does the same thing as `utils.FileTest(path, 'chardet')`.
- Added `utils.EditTextFile(path)` method. It uses the same editor as the one that is set for editing scripts in `window.ShowConfigureV2()`.
- Added `utils.FileExists(path)` method. Does the same thing as `utils.FileTest(path, 'e')`.
- Added `utils.GetFileSize(path)` method. Does the same thing as `utils.FileTest(path, 's')`.
- Added `utils.GetPackagePath(package_id)` method. Package id can be retrieved from `window.ScriptInfo` (only if the panel actually uses script package).
- Added `utils.IsDirectory(path)` method. Does the same thing as `utils.FileTest(path, 'd')`.
- Added `utils.IsFile(path)` method.
- Added `utils.SplitFilePath(path)` method. Does the same thing as `utils.FileTest(path, 'split')`.
- Added `window.DefineScript(script_name, { author: author_name, version: version_string, features: {drag_n_drop: boolean, grab_focus: boolean} })` method.
  - `grab_focus` has the same behaviour as the checkbox in the old `window.Configure()` dialog, i.e. if true, grabs user input when mouse is over the panel. `true` by default.
- Added `window.EditScript()` method. Uses editor that is set in `window.ShowConfigureV2()`. By default works the same as the old `window.Configure()` dialog.
- Added `window.ShowConfigureV2()` method. This dialog contains only various panel settings. Script editing is now invoked via `Edit` buttons or  `window.EditScript()`.
- Added `window.JsMemoryStats` property. 
  - `memory_usage`: same values as `window.PanelMemoryUsage`.
  - `total_memory_usage`: same values as `window.TotalMemoryUsage`.
  - `total_memory_limit`: same values as `window.MemoryLimit`.
- Added `window.ScriptInfo` property. 
  Contains all the info about the script that was either set up with `window.DefineScript()` or was defined in script package.

### Changed 
- `utils.FileTest()` is marked as **\[Deprecated]**. Use new corresponding methods instead.
- `window.ID` is now optional and unused in all methods that required it.
- `window.DefinePanel()` is marked as **\[Deprecated]**.
  Use `window.DefineScript()` instead.
  Note: to retain backward compatibility this method sets *both* script name and panel name.
- `window.MemoryLimit`, `window.PanelMemoryUsage` and `window.TotalMemoryUsage` are marked as **\[Deprecated]**. Use `window.JsMemoryStats` instead.
- `window.Name` now returns panel name instead of script name. Use `window.ScriptInfo.Name` to retrieve script name.
- `window.ShowConfigure()` is marked as **\[Deprecated]**. Use `window.ShowConfigureV2()` to configure panel and `window.EditScript` to edit script.

## v1.3.0

#### Added
- Added `GdiBitmap.InvertColours()` method: returns a `GdiBitmap` with inverted colours.
- Added `window.Tooltip` property: returns a `FbTooltip` object associated with the current panel.
- Added `FbTooltip.SetFont(font_name, font_size_px, font_style)` method: changes tooltip font.
- Added `ActiveXObject.ActiveX_CreateArray(arr, element_variant_type)` method: converts a JavaScript array to `ActiveXObject` that contains an object of type `VT_ARRAY|element_variant_type`.
  Arguments:
  - `arr`: a JS array that consists of elements of primitive type.
  - `element_variant_type` a numerial value of variant type (e.g. `0x11` for `VT_UI1`).

### Changed
- Updated SpiderMonkey JavaScript engine to 68.8.0 ESR:
  - ECMAScript 2019 conformant JavaScript. See ['What's new in ES2019'](https://flaviocopes.com/es2019) for more info.
- [**Deprecated**] `window.CreateTooltip()`: use `window.Tooltip` and `FbTooltip.SetFont()` instead.

## v1.2.2

### Added
- Added global constructor for `GdiFont`: same arguments as `gdi.Font()`.
- Added support for passing arguments to the callback in `setInterval` and `setTimeout` (see [JavaScript docs](https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/setInterval}) for usage example).

## v1.2.0

### Added
- Changed `include` behaviour:
  - `path` argument now supports relative paths.
  - Has `include guards` - script won't be evaluated a second time if it was evaluated before in the same panel.
  - Has script caching - script file will be read only once from filesystem (even if it is included from different panels).
- Added optional `options` argument to `include`, which might contain the following modifiers:
  - `always_evaluate` (default `false`): If true, evaluates the script even if it was included before.

## v1.1.2

### Added
- Added optional `options` argument to `fb.DoDragDrop`, which might contain the following modifiers:
  - `show_text` (default `true`): if true, will add track count text
  - `use_album_art` (default `true`): if true, will use album art of the focused item from dragged tracks (if available)
  - `use_theming` (default `true`): if true, will use Windows drag window style. Album art and custom image are resized to fit when Windows style is active.
  - `custom_image` (default `undefined`): custom dragging image. Will be also displayed if `use_album_art` is true, but there is no album art available.

## v1.1.0

### Added
- Added `clearInterval()`, `clearTimeout()`, `setInterval()`, `setTimeout()` methods to global namespace.  
  They work the same as corresponding `window.ClearInterval()`, `window.ClearTimeout()`, `window.SetInterval()`, `window.SetTimeout()` methods.
- Added `gdi.LoadImageSyncV2()` method.  
    Works the same as `gdi.LoadImageSync`, but returns a Promise object instead of calling `on_load_image_done()`.
- Added `utils.GetAlbumArtAsyncV2()` method.  
    Works the same as `gdi.GetAlbumArtAsync()`, but returns a Promise object instead of calling `on_get_album_art_done()`.
- Added arguments to `FbProfiler.Print()`: additional message and an option to disable component info.
- Added global constructors for the following objects:
  - `FbMetadbHandleList`: from another `FbMetadbHandleList`, from an array of `FbMetadbHandle`, from a single `FbMetadbHandle` and a default constructor.
  - `GdiBitmap`: from another `GdiBitmap`.
  - `FbProfiler`: accepts the same arguments as `fb.CreateProfiler()`.
  - `FbTitleFormat`: accepts the same arguments as `fb.TitleFormat()`.

### Changed
- `fb.DoDragDrop()` now requires an additional `window.ID` argument.
- New `Text` property in `action` argument of `on_drag_*` callbacks: setting this property will change the drop text on drag window.
- [**Deprecated**]`fb.CreateHandleList()`: use `FbMetadbHandleList` constructor instead.

## v1.0.5

### Added
- Added `window` properties for memory tracking:
  - `window.MemoryLimit`: maximum allowed memory usage for the component. If the total memory usage exceeds this value, all panels will fail with OOM error.
  - `window.PanelMemoryUsage`: memory usage of the current panel.
  - `window.TotalMemoryUsage`: total memory usage of all panels.

## v1.0.4

### Added
- Added `FbMetadbHandleList.RemoveAttachImages` method.

### Changed
- Rewrote `plman.PlaylistRecyclerManager`, since it was broken:
   - Replaced `Name` property with `GetName()` method.
   - Replaced `Content` property with `GetContent()` method.
   - Renamed to `plman.PlaylistRecycler`.

## v1.0.3

### Changed
- Reimplemented `utils.ShowHtmlDialog()`:
  - No longer considered **\[Experimental]** and is safe to use.
  - Does not return a value anymore.
  - Utilizes the latest non-Edge IE that you have on your system.
  - More options: `width`, `height`, `x`, `y`, `center`, `context_menu`, `resizable`, `selection`, `scroll`.

## v1.0.1

### Added
- **\[Experimental]** Added `utils.ShowHtmlDialog()` method.  
  This method allows the creation of modal HTML windows rendered by IE8 engine.  
  `utils.ShowHtmlDialog(window_id, code_or_file, { width: window_w, height: window_h, data: callback_data })`.  
  - `code_or_file`: either html code or path to html file (must be set with `file://` prefix).  
  - `data`: will be saved in `window.external.dialogArguments` object and can be accessed from JavaScript executed inside HTML window.
  - Method returns value set to `window.returnValue` from HTML code.

### Removed
- Removed `utils.CreateHtmlWindow()` method, since it was totally broken.

## v1.0.0

Note: despite changing scripting engine to `SpiderMonkey`, the file-system and web access is still provided via ActiveX objects.

### Added
- JavaScript is now conformant to ES2018 standard and has quite a few new features:
   - [Full ES2015-ES2018 overview](https://flaviocopes.com/ecmascript/).
   - [ES2015/ES6](http://es6-features.org).
   - [ES2016/ES7](https://medium.freecodecamp.org/ecmascript-2016-es7-features-86903c5cab70).
   - [ES2017/ES8](https://edgecoders.com/whats-new-in-es2017-or-es8-for-javascript-40352b089780).
   - [ES2018/ES9](https://medium.com/front-end-hacking/javascript-whats-new-in-ecmascript-2018-es2018-17ede97f36d5).
- All methods that returned `VBArray` before return a proper JS array now.
- `FbMetadbHandleList` items now can be accessed with [ ] operator.
- Added global `include('path/to/script.js')` method.  
  This method is like `eval(ReadFile('path/to/script.js'))`, but provides better error reporting and might have more features in the future (like script caching or include guards).
- Added `window.DefinePanel()` method.  
  A replacement for the old `==PREPROCESSOR==` header.  
  `window.DefinePanel(panel_name, { author: author_name, version: version_string, features: {drag_n_drop: boolean} } )`.  
  - The second argument and all of it's properties are optional.  
  - `drag_n_drop` is `false` by default.
- **\[BROKEN, DON'T USE]** Added `utils.CreateHtmlWindow()` method.

### Breaking changes

| Change | Reason | Workaround |
| ------------- | -------------| ----- |
| Dropped support for Windows XP/Vista (Windows 7 is the minimum requirement) | SpiderMonkey engine and CUI SDK require Windows 7 or higher | Use [foo_jscript_panel](https://github.com/marc2k3/foo_jscript_panel) or upgrade your OS |
| All methods and properties are case-sensitive | Required by ECMAScript standard | Fix the wrong casing :) |
| Removed `Dispose()` | Not needed anymore: JS GC destroys all objects by itself when corresponding reference count is zero | Remove those methods and assign `null` or `undefined` to variables if needed |
| Removed `toArray()` | Not needed anymore: all corresponding methods return a proper JS array now | Remove those methods |
| Removed `FbMetadbHandleList.Item()` | Not needed anymore: `FbMetadbHandleList` has proper array accessors now | Replace `.Item(i)` with `[i]` |
| Changed `utils.Version` return type: returns `string` instead of `number` | Component uses [semantic versioning](https://semver.org/), which requires string representation | Remove all version checks - the versioning is different from JSP anyway | 
| New limitations imposed on `on_notify_data` callback regarding `data` argument(see [docs][callbacks_js]) | Limitations are brought by the JS engine | Don't modify `data` and make a deep clone when storing it |
| Removed old `==PREPROCESSOR==` panel header support | Old header could only be used in the main configure panel and was not a proper JS method | Replace with the following:<br>`window.DefinePanel('Panel Name', { author: 'Author', version: 'Version', features: {drag_n_drop: true|false}} );` |
| Most methods have much stricter error checks | Before, methods could fail silently without doing anything, which made diagnosing errors unnecessary hard | Fix incorrect calls and arguments |

[callbacks_js]: https://github.com/TheQwertiest/foo_spider_monkey_panel/blob/master/component/docs/Callbacks.js
