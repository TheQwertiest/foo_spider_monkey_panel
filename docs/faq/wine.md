---
title: WINE issues & workarounds
parent: Frequesntly Asked Questions
nav_order: 1
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

- **Issue**: `action.Effect` in `on_drag_*` is equal to zero (`DROPEFFECT_NONE`) when performing DnD on some files (e.g. when dropping files from Nautilus).  
  **Reason**: This is a WINE bug: they just don't fill the field with proper values.

- **Issue**: `.otf` fonts are not found via `utils.CheckFont()` and `gdi.Font()`.  
  **Reason**: `gdiplus` package does not support `.otf` fonts.
