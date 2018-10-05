# Changelog

## [Unreleased]
### Added
- Integrated `foo_acfu` update checks.

### Changed
- **[Experimental]** Replaced `utils.CreateHtmlWindow()` with `utils.ShowHtmlDialog()`.
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

## [1.0.0] - 2018-10-01
### Added
- Added stack trace to error reports.
- API changes:
  - JavaScript is now conformant to ES2017 standard.
  - All arrays now can be accessed directly with [] operator (no need for `toArray()` cast).
  - FbMetadbHandleList items now can be accessed with [] operator.
  - Added global `include()` method.
  - Added `window.DefinePanel` method.
  - **[Experimental]** Added `utils.CreateHtmlWindow()` method.

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

[Unreleased]: https://github.com/theqwertiest/foo_spider_monkey_panel/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/TheQwertiest/foo_spider_monkey_panel/compare/vanilla_2_0...v1.0.0
