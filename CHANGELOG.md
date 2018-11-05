# Changelog

#### Table of Contents
- [Unreleased](#unreleased)
- [1.0.5](#104---2018-11-06)
- [1.0.4](#104---2018-10-25)
- [1.0.3](#103---2018-10-11)
- [1.0.2](#102---2018-10-05)
- [1.0.1](#101---2018-10-05)
- [1.0.0](#100---2018-10-01)
___

## [Unreleased][]

## [1.0.5][] - 2018-11-06
### Added
- Added basic handling of exception-like objects and objects derived from `Error` in pre-ES6 style.
- Added handling of unhandled rejected promises.
- Added properties to `window` for memory usage tracking: `MemoryLimit`, `TotalMemoryUsage` and `PanelMemoryUsage`.

### Fixed
- Fixed timing of promises invocation: now conforms to ES standard.
- Fixed several memory leaks on panel layout switch and on script error.
- Fixed crash on in-panel layout switch.

## [1.0.4][] - 2018-10-25
### Added
- Added HTML documentation.
- Improved error reports of component startup failures.
- Ported JScript Panel changes:
  - Added `FbMetadbHandleList.RemoveAttachImages` method.

### Fixed
- Fixed crash when `on_main_menu` callback was invoked.
- Fixed crash when switching layout from inside the panel.
- Fixed occasional crash on panel removal.
- Fixed incorrect handling of UTF-16 BOM files in `include()` and `utils.ReadTextFile()`.
- Fixed `ThemeManager.DrawThemeBackground()`: was ignoring `state_id` argument.
- Fixed invalid calculation of image size, which resulted in premature OOM errors.

### Changed
- Improved `include` performance by 2x.
- Tweaked GC for better UX during high load.
- Rewrote `plman.PlaylistRecyclerManager`, since it was broken:
  - Replaced `Name` property with `GetName` method.
  - Replaced `Content` property with `GetContent` method.
  - Renamed to `plman.PlaylistRecycler`.
- Rewrote `Interfaces.js`
  - Fixed invalid and incorrect JSDoc tags.
  - Renamed to `foo_spider_monkey_panel.js`.

## [1.0.3][] - 2018-10-11
### Changed
- Reimplemented `utils.ShowHtmlDialog()`. It's no longer considered **\[Experimental]** and is safe to use.
- Updated `Interfaces.js`:
  - Updated `utils.ShowHtmlDialog()` doc.
  - Updated `fb.DoDragDrop()` doc.
  - Updated `fb.IsMainMenuCommandChecked()` doc.
- Updated `ActiveXObject.js`: added info on helper methods `ActiveX_Get`/`ActiveX_Set`.

## [1.0.2][] - 2018-10-05
### Fixed
- Fixed regression in `ActiveXObject` handling.

## [1.0.1][] - 2018-10-05
### Added
- Integrated `foo_acfu` update checks.
- Added Scintilla line-wrap settings to `Preferences > Tools > Spider Monkey Panel`.

### Changed
- **\[Experimental]** Replaced `utils.CreateHtmlWindow()` with `utils.ShowHtmlDialog()`.
- Updated `Interfaces.js`:
  - Updated `window.GetProperty()`/`window.SetProperty()` docs.
  - Updated `utils.Version` doc.
  - Updated `FbMetadbHandleList.UpdateFileInfoFromJSON` doc.

### Fixed
- Disabled callback invocation until script is fully evaluated.
- Fixed `fb.RunContextCommandWithMetadb()`.
- Fixed samples:
  - Removed left-over `.Dispose()` calls.
  - Rewrote html dialog sample (HtmlDialogWithCheckBox.txt).
- Added a few fixes to `ActiveXObject` for better compatibility.

## [1.0.0][] - 2018-10-01
### Added
- Added stack trace to error reports.
- API changes:
  - JavaScript is now conformant to ES2017 standard.
  - All arrays now can be accessed directly with [] operator (no need for `toArray()` cast).
  - FbMetadbHandleList items now can be accessed with [] operator.
  - Added global `include()` method.
  - Added `window.DefinePanel` method.
  - **\[Experimental]** Added `utils.CreateHtmlWindow()` method.

### Changed
- Rewrote component to use Mozilla SpiderMonkey JavaScript engine.
- Windows 7 is the minimum supported OS now.
- API changes:
  - All methods and properties are case-sensitive as required by ECMAScript standard.
  - Removed `Dispose()` and `toArray()` methods.
  - Removed `FbMetadbHandleList.Item()` method.
  - Removed old `==PREPROCESSOR==` panel header support.
  - `utils.Version` returns string instead of number.
  - More rigorous error checks.
- Updated samples with compatibility fixes.

[unreleased]: https://github.com/theqwertiest/foo_spider_monkey_panel/compare/v1.0.5...HEAD
[1.0.4]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/vanilla_2_0...v1.0.0
