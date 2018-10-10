# Changelog

#### Table of Contents
- [Unreleased](#unreleased)
- [1.0.2](#102---2018-10-05)
- [1.0.1](#101---2018-10-05)
- [1.0.0](#100---2018-10-01)
___

## [Unreleased][]
### Changed
- Rewrote `utils.ShowHtmlDialog()`. It's no longer considered **\[Experimental]** and is safe to use.
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

[unreleased]: https://github.com/theqwertiest/foo_spider_monkey_panel/compare/v1.0.2...HEAD
[1.0.2]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/vanilla_2_0...v1.0.0
