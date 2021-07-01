---
title: WINE issues & workarounds
nav_order: 7
---

# WINE issues & workarounds
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

## Fixable issues

- **Issue**:  
```
ActiveXObject_Constructor failed:
Failed to create ActiveXObject object via CLSID: htmlfile
```
  **Fix**: install [Gecko package](https://wiki.winehq.org/Gecko).

- **Issue**: fonts are not found via `utils.CheckFont()` and `gdi.Font()`.  
  **Fix**: install `gdiplus` package via `winetricks`.

## Unfixable issues

- **Issue**: `on_mouse_leave` event might stop triggering in some cases.  
  **Reason**: WINE bug? [Issue #60](https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/60).

- **Issue**: `.otf` fonts are not found via `utils.CheckFont()` and `gdi.Font()`.  
  **Reason**: `gdiplus` package does not support `.otf` fonts.
