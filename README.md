# foo_uie_wsh_panel_mod
foobar2000 component

All credit to T.P Wang for the official version which is available here: https://code.google.com/p/foo-wsh-panel-mod/downloads/list

##Important

Please read the changelog below and pay special attention to the features removed in `v1.5.7`. These were marked as obsolete well over 3 years ago and the official component `v1.5.6` already reports them as such in the `foobar2000` `Console`. If any of your scripts are triggering these warnings, they will not even run in this component. The scripts will need updating. Script authors should check `preprocessors.txt` and `interfaces.txt` in the docs. For anyone not capable of fixing their own scripts, they should continue to use the official component.

As of `v1.5.11`, Windows XP is no longer supported. `v1.5.10` is still available on the [releases](https://github.com/19379/foo_uie_wsh_panel_mod/releases) page.

```
v1.6.2
- ADD: plman.AddLocations(playlistIndex, paths[, select]
       paths: an array of file paths and/or URLs
       select: false if omitted.

v1.6.1.2
- CHG: Large docs update to interfaces.txt and callbacks.txt

v1.6.1.1
- FIX: Fix minor errors in some sample script comments.
- CHG: Change script error background colour.

v1.6.1
- ADD: window.CreateToolTip now takes optional font name, font size (px) and
       style arguments. eg window.CreateToolTip("Segoe UI", 32, 1);
       Defaults of "Segoe UI", 12 and 0 are used if omitted. See docs\flags.txt
       for valid style values.
- ADD: plman.GetQueryItems(source_handlelist, query) returns an unsorted
       handle list. Consider using OrderByFormat, etc on the result
- ADD: plman.CreateAutoplaylist, plman.IsAutoPlaylist
       These behave the same as the "fb" methods which already exist.
- ADD: plman.ClearPlaylist(playlistIndex). Unlike fb.ClearPlaylist which
       clears the active playlist, this requires a "playlistIndex"
       argument.
- CHG: "Safe mode" is no longer an option.
- CHG: Tidy up samples.

v1.6.0.1
- CHG: Remove on_tooltip_custom_paint() callback.

v1.6.0
- ADD: If "import" files listed in the "preprocessor" section are missing, a popup
       window will notify you.
- CHG: Remove these duplicate functions because "plman" alternatives also exist.

       fb.CreatePlaylist
       fb.RemovePlaylist
       fb.RenamePlaylist
       fb.DuplicatePlaylist
       fb.MovePlaylist
       fb.ActivePlaylist
       fb.PlayingPlaylist
       fb.PlaylistCount
       fb.PlaylistItemCount
       fb.GetPlaylistName

       Simply replace "fb" with "plman" in any affected scripts. Scripts updated
       with these changes will still work with the official component "v1.5.0"
       and above.

       I've updated my latest scripts to be compliant. Get "v11" or later from here:

       https://github.com/19379/wsh_marc2003/releases

       I've also updated Br3tt's "JSplaylist":

       https://github.com/19379/jsplaylist-mod
- CHG: Tidy up internal preprocessor handling. Also, some other obsolete code has been
       removed.

v1.5.12
- FIX: "Reset page" button in "Preferences" now turns "Safe Mode" off
       to be consistent with the changes to default behaviour introduced
       in "v1.5.7".
- CHG: "Properties" dialog has a larger default size.

v1.5.11
- FIX: IFbMetadbHandle FileSize now works with "JScript" engine. Previously,
       it only worked with "JScript9".
- CHG: Windows XP is no longer supported.

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
