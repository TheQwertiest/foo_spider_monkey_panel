# foo_uie_wsh_panel_mod
foobar2000 component

All credit to T.P Wang for the official version which is available here: https://code.google.com/p/foo-wsh-panel-mod/downloads/list

##Important

Please read the changelog below and pay special attention to the features removed in `v1.5.7`. These were marked as obsolete well over 3 years ago and the official component `v1.5.6` already reports them as such in the `foobar2000` `Console`. If any of your scripts are triggering these warnings, they will not even run in this component. The scripts will need updating. Script authors should check `preprocessors.txt` and `interfaces.txt` in the docs. For anyone not capable of fixing their own scripts, they should continue to use the official component.


My changes:

```
v1.5.10
- ADD: plman.UndoBackup(playlistIndex). If you call this before using other
       plman methods to add/remove/reorder playlist items, you will
       be able to use the undo/redo commands on the Edit menu.
- CHG: Tidy up samples and add missing images. Paths to required files
       are now relative.

v1.5.9
- CHG: on_library_changed() has been deprecated after a very short life.
- ADD: on_library_items_added()
- ADD: on_library_items_removed()
- ADD: on_library_items_changed()

v1.5.8
- ADD: fb.IsLibraryEnabled()

v1.5.8 Beta 1
- ADD: fb.ShowLibrarySearchUI(query) opens the Library>Search window
       populated with the query you set.
- ADD: fb.GetLibraryItems() returns a handle list of all items in library.
- ADD: on_library_changed callback for when library items are added,
       removed or modified.

v1.5.7.1
- ADD: window.Reload() so you can force a panel reload from your own menus,
       buttons, functions etc.

v1.5.7
- CHG: Compiled with new SDK. Requires foobar2000 v1.3 or above.
- ADD: Script errors are now displayed in a popup window in addition to
       the Console like it was previously.
- ADD: Default right click menu now has a "Reload" script option. This
       saves opening/closing the dialog when working on external files.
- CHG: Remove functions marked as obsolete 2+ years ago. There are newer
       alternatives for all of them:

       window.WatchMetadb
       window.UnWatchMetadb
       window.CreateTimerTimeout
       window.CreateTimerInterval
       window.KillTimer
       UpdateFileInfo
- CHG: AppendMenuItem no longer accepts MF_POPUP as a flag. You should be
       using AppendTo instead.
- CHG: utils.GetAlbumArt removed as corresponding function has been
       removed from SDK.
- CHG: Safe mode disabled by default. If you're reading this, you're
       probably going to be using scripts that require this!
- FIX: EstimateLineWrap no longer leaves stray punctuation when wrapping
       text at end of line.
```
