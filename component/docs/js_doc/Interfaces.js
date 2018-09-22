/**
 * Evaluates the script in file.
 * Similar to eval(ReadFile(path)), but provides better error reporting
 *
 * @param {string} path Path to JavaScript file
 */
function include(path) {};

/**
 * @constructor
 */
function IFbUtils() {
    /** @type {boolean} */
    this.AlwaysOnTop = undefined; //(boolean) (read, write)
    /*
    Example:
    fb.AlwaysOnTop = !fb.AlwaysOnTop; // Toggles the current value.
    */

    /** @type {string} */
    this.ComponentPath = undefined; // (string) (read)
    /*
    Example:
    console.log(fb.ComponentPath); // C:\Users\User\AppData\Roaming\foobar2000\user-components\foo_spider_monkey_panel\
    */

    /** @type {boolean} */
    this.CursorFollowPlayback = undefined; // (boolean) (read, write)

    /** @type {string} */
    this.FoobarPath = undefined; // (string) (read)

    /** @type {boolean} */
    this.IsPaused = undefined; // (boolean) (read)

    /** @type {boolean} */
    this.IsPlaying = undefined; // (boolean) (read)

    /** @type {boolean} */
    this.PlaybackFollowCursor = undefined; // (boolean) (read, write)

    /** @type {float} */
    this.PlaybackLength = undefined; // (double) (read)
    /*
    Example1:
    console.log(fb.PlaybackLength); // 322.843414966166

    Example2:
    console.log(Math.round(fb.PlaybackLength)); // 323
    */

    /** @type {float} */
    this.PlaybackTime = undefined; // (double) (read, write)
    /*
    Example:
    fb.PlaybackTime = 60; // Jumps to the 1 minute mark.
    */

    /** @type {string} */
    this.ProfilePath = undefined; // (string) (read)

    /**
     * 0 - None
     * 1 - Track
     * 2 - Album
     * 3 - Track/Album by Playback Order (only available in foobar2000 v1.3.8 and later)
     *
     * @type {number}
     */
    this.ReplaygainMode = undefined; // (uint) (read, write)

    /** @type {boolean} */
    this.StopAfterCurrent = undefined; // (boolean) (read, write)
    /*
    Example:
    fb.StopAfterCurrent = !fb.StopAfterCurrent; // Toggles the current value.
    */

    /** @type {float} */
    this.Volume = undefined; // (float) (read, write);
    /*
    Example:
    fb.Volume = 0; // Sets the volume to max. -100 is the minimum.
    */

    /**
     * @return {IFbUiSelectionHolder}
     */
    this.AcquireUiSelectionHolder = function () {}; // (IFbUiSelectionHolder)

    /**
     * This is typically used to update the selection used by the default UI artwork panel
     * or any other panel that makes use of the preferences under
     * File>Preferences>Display>Selection viewers. Use in conjunction with the on_focus
     * callback. See Callbacks.js.
     *
     * @constructor
     */
    function IFbUiSelectionHolder() {

        /**
         * Sets the selected items.
         *
         * @param {IFbMetadbHandleList} handle_list
         */
        this.SetSelection = function (handle_list) {}; // (void)

        /**
         * Sets selected items to playlist selection and enables tracking.
         * When the playlist selection changes, the stored selection is automatically
         * updated. Tracking ends when a set method is called on any ui_selection_holder
         * or when the last reference to this ui_selection_holder is released.
         */
        this.SetPlaylistSelectionTracking = function () {}; // (void)

        /**
         * Sets selected items to playlist contents and enables tracking.
         * When the playlist selection changes, the stored selection is automatically
         * updated. Tracking ends when a set method is called on any ui_selection_holder
         * or when the last reference to this ui_selection_holder is released.
         */
        this.SetPlaylistTracking = function () {}; // (void)

    }

    /*
    Example1: (for playlist viewers)

    var selection_holder = fb.AcquireUiSelectionHolder();
    selection_holder.SetPlaylistSelectionTracking();

    function on_focus(is_focused) {
        if (is_focused) { // Updates the selection when panel regains focus
            selection_holder.SetPlaylistSelectionTracking();
        }
    }

    Example2: (for library viewers)

    var selection_holder = fb.AcquireUiSelectionHolder();
    var handle_list = null;

    function on_mouse_lbtn_up(x, y) { // Presumably going to select something here...
        handle_list = ...;
        selection_holder.SetSelection(handle_list);
    }

    function on_focus(is_focused) {
        if (is_focused) { // Updates the selection when panel regains focus
            if (handle_list && handle_list.Count)
                selection_holder.SetSelection(handle_list);
        }
    }
    */

    this.AddDirectory = function () {}; // (void)

    this.AddFiles = function () {}; // (void)

    /**
     * Checks Clipboard contents are handles or a file selection from Windows Explorer. Use in conjunction
     * with fb.GetClipboardContents().
     *
     * @param {number} window_id
     * @return {boolean}
     */
    this.CheckClipboardContents = function (window_id) {}; // (boolean)

    /**
     * Clears active playlist. If you wish to clear a specific playlist, use plman.ClearPlaylist(playlistIndex).
     */
    this.ClearPlaylist = function () {}; // (void)

    /**
     * @param {IFbMetadbHandleList} handle_list
     * @return {boolean}
     */
    this.CopyHandleListToClipboard = function (handle_list) {}; // (boolean)
    /*
    Items can then be pasted in other playlist viewers or in Windows Explorer as files.

    Example1: (copy playlist items)
    var handle_list = plman.GetPlaylistSelectedItems(plman.ActivePlaylist);
    fb.CopyHandleListToClipboard(handle_list);

    Example2: (cut playlist items)
    var ap = plman.ActivePlaylist;
    if (!plman.IsPlaylistLocked(ap)) {
        var handle_list = plman.GetPlaylistSelectedItems(ap);
        if (fb.CopyHandleListToClipboard(handle_list)) {
            plman.UndoBackup(ap);
            plman.RemovePlaylistSelection(ap);
        }
    }
    */

    /**
     * See samples\basic\MainMenuManager All-In-One, samples\basic\Menu Sample.txt
     *
     * @return {IContextMenuManager}
     */
    this.CreateContextMenuManager = function () {}; // (IContextMenuManager)

    /**
     * @constructor
     */
    function IContextMenuManager() {
        /**
         * @param {IMenuObj} menuObj
         * @param {number} base_id
         * @param {number=} [max_id=-1]
         */
        this.BuildMenu = function (menuObj, base_id, max_id) {}; // (void)

        /**
         * @param {number} id
         * @return {boolean}
         */
        this.ExecuteByID = function (id) {}; // (boolean)

        /**
         * @param {IFbMetadbHandleList} handle_list
         */
        this.InitContext = function (handle_list) { }; // (void)

        /**
         * Shows playlist specific options that aren't available when passing a
         * handle list to InitContext.
         */
        this.InitContextPlaylist = function () { }; // (void)

        this.InitNowPlaying = function () {}; // (void)
    }

    /**
     * Returns an empty handle list.
     *
     * @return {IFbMetadbHandleList}
     */
    this.CreateHandleList = function () {}; // (IFbMetadbHandleList)

    /**
     * See samples\basic\MainMenuManager All-In-One, samples\basic\Menu Sample.txt
     *
     * @return {IMainMenuManager}
     */
    this.CreateMainMenuManager = function () {}; // (IMainMenuManager)

    /**
     * @constructor
     */
    function IMainMenuManager() {
        /**
         * @param {IMenuObj} menuObj
         * @param {number} base_id
         * @param {number} count
         */
        this.BuildMenu = function (menuObj, base_id, count) {}; // (void)

        /**
         * @param {number} id
         * @return {boolean}
         */
        this.ExecuteByID = function (id) {}; // (boolean)

        this.Init = function (root_name) {}; // (void)
    }

    /**
     * @param {string=} name Will be shown in console when used with Print() method below
     * @return {IFbProfiler}
     */
    this.CreateProfiler = function (name) {}; // (IFbProfiler) [name]

    /**
     * @constructor
     */
    function IFbProfiler() {

        /** @type {number} */
        this.Time = undefined; // (int) // milliseconds

        this.Reset = function () {}; // (void)
        this.Print = function () {}; // (void)
    }

    /*
    Example:
    var test = fb.CreateProfiler("test");
    // do something time consuming
    console.log(test.Time); // Outputs bare time in ms like "789"
    test.Print(); // Outputs component name/version/assigned name like "Spider Monkey Panel v1.0.0: FbProfiler (test): 789 ms"
    */

    /**
     * See {@link https://msdn.microsoft.com/en-us/library/windows/desktop/ms678486.aspx} and {@link https://github.com/marc2k3/foo_jscript_panel/wiki/Drag-and-Drop}
     *
     * @param {IFbMetadbHandleList} handle_list
     * @param {number} effect Allowed effects.
     * @return {number} Effect that was returned in on_drag_drop.
     */
    this.DoDragDrop = function (handle_list, effect) {}; // (uint);

    this.Exit = function () {}; // (void)

    /**
     * @param {number} window_id
     * @return {IFbMetadbHandleList}
     */
    this.GetClipboardContents = function (window_id) {}; // (IFbMetadbHandleList)
    /*
    Clipboard contents can be handles copied to the clipboard in other components, from fb.CopyHandleListToClipboard or a file selection
    from Windows Explorer etc.

    Example:
    function on_mouse_rbtn_up(x, y) {
        var ap = plman.ActivePlaylist;
        var menu = window.CreatePopupMenu();
        menu.AppendMenuItem(!plman.IsPlaylistLocked(ap) && fb.CheckClipboardContents(window.ID) ? MF_STRING : MF_GRAYED, 1, "Paste"); // see Flags.js for MF_* definitions
        var idx = menu.TrackPopupMenu(x, y);
        if (idx == 1) {
            var handle_list  = fb.GetClipboardContents(window.ID);
            plman.InsertPlaylistItems(ap, plman.PlaylistItemCount(ap), handle_list );
        }
        return true;
    }
    */

    /**
     * @param {boolean=} [force=true] When true, it will use the first item of the active playlist if it is unable to get the focus item.
     * @return {IFbMetadbHandle}
     */
    this.GetFocusItem = function (force) {}; // (IFbMetadbHandle) [force]

    /**
     * Returns all Media Library items as a handle list.
     *
     * @return {IFbMetadbHandleList}
     */
    this.GetLibraryItems = function () {}; // (IFbMetadbHandleList)

    /**
     * NOTE: Do not use this while looping a handle list. Use {@link IFbMetadbHandleList.GetLibraryRelativePaths} instead.
     *
     * Returns an empty string when used on track not in Media Library
     *
     * @param {IFbMetadbHandle} handle
     * @return {string}
     */
    this.GetLibraryRelativePath = function (handle) {}; // (string)
    /*
    Example:
    The foobar2000 Media Library is configured to watch "D:\Music" and the
    path of the now playing item is "D:\Music\Albums\Artist\Some Album\Some Song.flac"

    var handle = fb.GetNowPlaying();
    console.log(fb.GetLibraryRelativePath(handle)); // Albums\Artist\Some Album\Some Song.flac
    */

    /**
     * Get handle of now playing item.
     *
     * @return {?IFbMetadbHandle} null, if nothing is being played.
     */
    this.GetNowPlaying = function () {}; // (IFbMetadbHandle)

    /**
     * foobar2000 v1.4 and above only. Throws a script error on v1.3.
     * Returns a JSON array in string form so you need to use JSON.parse() on the result.
     *
     * @return {string}
     */
    this.GetOutputDevices = function () {}; // (string)
    /*
    Example:
    var str = fb.GetOutputDevices();
    var arr = JSON.parse(str);
    console.log(JSON.stringify(arr, null, 4));

    [
        {
            "active": false,
            "device_id": "{5243F9AD-C84F-4723-8194-0788FC021BCC}",
            "name": "Null Output",
            "output_id": "{EEEB07DE-C2C8-44C2-985C-C85856D96DA1}"
        },
        {
            "active": true,
            "device_id": "{00000000-0000-0000-0000-000000000000}",
            "name": "Primary Sound Driver",
            "output_id": "{D41D2423-FBB0-4635-B233-7054F79814AB}"
        },
        {
            "active": false,
            "device_id": "{1C4EC038-97DB-48E7-9C9A-05FDED46847B}",
            "name": "Speakers (Sound Blaster Z)",
            "output_id": "{D41D2423-FBB0-4635-B233-7054F79814AB}"
        },
        {
            "active": false,
            "device_id": "{41B86272-3D6C-4A5A-8907-4FE7EBE39E7E}",
            "name": "SPDIF-Out (Sound Blaster Z)",
            "output_id": "{D41D2423-FBB0-4635-B233-7054F79814AB}"
        },
        {
            "active": false,
            "device_id": "{9CDC0FAE-2870-4AFA-8287-E86099D69076}",
            "name": "3 - BenQ BL3200 (AMD High Definition Audio Device)",
            "output_id": "{D41D2423-FBB0-4635-B233-7054F79814AB}"
        }
    ]

    As you can see, only one of the items in the array has "active"
    set to true so that is the device you'd want to display the name of
    or mark as selected in a menu.

    To actually change device, you'll need the device_id and output_id
    and use them with fb.SetOutputDevice.

    Example:
    var str = fb.GetOutputDevices();
    var arr = JSON.parse(str);
    // Assuming same list from above, switch output to the last device.
    fb.SetOutputDevice(arr[4].output_id, arr[4].device_id);
    */

    /**
     * @param {IFbMetadbHandleList} handle_list
     * @param {string} query
     * @return {IFbMetadbHandleList}
     */
    this.GetQueryItems = function (handle_list, query) {}; // (IFbMetadbHandleList)
    /*
    Example1:
    var a = fb.GetQueryItems(plman.GetPlaylistItems(plman.ActivePlaylist), "rating IS 5");
    Example2:
    var b = fb.GetQueryItems(fb.GetLibraryItems(), "rating IS 5");
    Results are unsorted.

    NOTE: Use try/catch to handle invalid queries. An empty handle list will be returned if the query
    is valid but there are no results.
    */

    /**
     * Gets now playing or selected item. What you get will depend on
     * "File>Preferences>Display>Selection viewers".
     *
     * @return {?IFbMetadbHandle}
     */
    this.GetSelection = function () {}; // (IFbMetadbHandle)

    /**
     * Works like GetSelection(), but returns a handle list.
     * Always returns a valid handle list instance instead of null.
     *
     * @param {number=} [flags=0] 1 - no now playing
     * @return {IFbMetadbHandleList}
     */
    this.GetSelections = function (flags) {}; // (IFbMetadbHandleList) //[flags]

    /**
     * Retrieves what the selection type is.
     *
     * @return {number}
     *     0 - undefined (no item)
     *     1 - active_playlist_selection
     *     2 - caller_active_playlist
     *     3 - playlist_manager
     *     4 - now_playing
     *     5 - keyboard_shortcut_list
     *     6 - media_library_viewer
     */
    this.GetSelectionType = function () {}; // (uint)

    /**
     * @return {boolean}
     */
    this.IsLibraryEnabled = function () {}; // (boolean)

    /**
     * @param {IFbMetadbHandle} handle
     * @return {boolean}
     */
    this.IsMetadbInMediaLibrary = function (handle) {}; // (boolean)
    /*
    Example:
    var np = fb.GetNowplaying();
    console.log(fb.IsMetadbInMediaLibrary(np)); // If false, playing track is not in Media Library.
    */

    this.LoadPlaylist = function () {}; // (void)

    this.Next = function () {}; // (void)

    this.Pause = function () {}; // (void)

    this.Play = function () {}; // (void)

    this.PlayOrPause = function () {}; // (void)

    this.Prev = function () {}; // (void)

    this.Random = function () {}; // (void)

    /**
     * For now playing file only.
     *
     * @param {string} command
     * @param {number=} [flags=0]
     *     0 - default (depends on whether SHIFT key is pressed, flag_view_reduced or flag_view_full is selected)
     *     4 - flag_view_reduced
     *     8 - flag_view_full. This can be useful if you need to run context commands the user may have hidden
     *         using File>Preferences>Display>Context Menu
     * @return {boolean}
     */
    this.RunContextCommand = function (command, flags) {}; // (boolean) [, flags]
    /*
    Example:
    fb.RunContextCommand("Properties");
    */

    /**
     * @param {string} command
     * @param {IFbMetadbHandle|IFbMetadbHandleList} handle_or_handle_list
     * @param {number=} flags Same flags as {@link plman.RunContextCommand}
     * @return {boolean}
     */
    this.RunContextCommandWithMetadb = function (command, handle_or_handle_list, flags) {}; // (boolean) [, flags]
    // handle_or_handle_list can be something like fb.GetFocusItem()
    // or plman.GetPlaylistSelectedItems(plman.ActivePlaylist)

    /**
     * @param {string} command
     * @return {boolean}
     */
    this.RunMainMenuCommand = function (command) {}; // (boolean)
    /*
    Example:
    fb.RunMainMenuCommand("File/Add Location...");
    */

    /**
     * @param {string} command
     * @return {boolean}
     */
    this.IsMainMenuCommandChecked = function (command) {}; // (boolean)

    this.SavePlaylist = function () {}; // (void)

    /**
     * foobar2000 v1.4 and above only. Throws a script error on v1.3.
     * See {@link fb.GetOutputDevices}.
     *
     * @param {string}
     * @param {string}
     */
    this.SetOutputDevice = function (output, device) {}; // (void)

    this.ShowConsole = function () {}; // (void)

    /**
     * Opens the Library>Search window populated with the query you set.
     *
     * @param {string} query
     */
    this.ShowLibrarySearchUI = function (query) {}; // (void)

    /**
     * @param {string} message
     * @param {string=} [title='Spider Monkey Panel']
     */
    this.ShowPopupMessage = function (message, title) {}; // (void) [, title]

    this.ShowPreferences = function () {}; // (void)

    this.Stop = function () {}; // (void)

    /**
     * @param {string} expression
     * @return {IFbTitleFormat}
     */
    this.TitleFormat = function (expression) {}; // (IFbTitleFormat)

    /**
     * @constructor
     */
    function IFbTitleFormat() {
        /*
        This will be used in the examples below:
        var tfo = fb.TitleFormat("%artist%");
        */

        /**
         * @param {boolean=} [force=false] If true, you can process text that doesn't contain
         *     title formatting even when foobar2000 isn't playing. When playing, you
         *     should always get a result.
         * @return {string}
         */
        this.Eval = function (force) {}; // [force]
        /*
        Always use Eval when you want dynamic info such as %playback_time%, %bitrate% etc.
        EvalWithMetadb(fb.GetNowplaying()) will not give the results you want.
        Example:
        console.log(tfo.Eval());
        */

        /**
         * @param {IFbMetadbHandle} handle
         * @return {string}
         */
        this.EvalWithMetadb = function (handle) {}; //
        /*
        Example:
        console.log(tfo.EvalWithMetadb(fb.GetFocusItem()));
        */

        /**
         * @param handle_list {IFbMetadbHandleList}
         * @return {Array<string>}
         */
        this.EvalWithMetadbs = function (handle_list) {}; //(Array)
        /*
        Example:
        var handle_list = fb.GetLibraryItems();
        var artists = tfo.EvalWithMetadbs(handle_list);
        console.log(handle_list.Count === artists.length); // should always be true!
        */
    }

    this.VolumeDown = function () {}; // (void)

    this.VolumeMute = function () {}; // (void)

    this.VolumeUp = function () {}; // (void)
}

var fb = new IFbUtils();

/**
 * @constructor
 */
function IGdiUtils() {

    /**
     * @param {number} w
     * @param {number} h
     * @return {IGdiBitmap}
     */
    this.CreateImage = function (w, h) {}; // (IGdiBitmap)

    /**
     * Avoid using inside on_paint.
     *  
     * @param {string} name
     * @param {number} size_px See Helper.js > Point2Pixel function for conversions
     * @param {number=} [style=0] See Flags.js > FontStyle
     * @return {?IGdiFont} Returns null if font is not present.
     */
    this.Font = function (name, size_px, style) {}; // (IGdiFont) [, style]

    /**
     * @param {string} path
     * @return {?IGdiBitmap} null if path doesn't exist or image fails to load.
     */
    this.Image = function (path) {}; // (IGdiBitmap)
    /*
    Example:
    var img = "e:\\images folder\\my_image.png";
    */

    /**
     * @param {number} window_id
     * @param {string} path
     * @return {number} Returns a unique id.
     */
    this.LoadImageAsync = function (window_id, path) {}; // (uint)
}

var gdi = new IGdiUtils();

/**
 * @constructor
 */
function IFbPlaylistManager() {

    /** @type {number} */
    this.ActivePlaylist = undefined; // (int) (read, write)
    /*
    Example1:
    console.log(plman.ActivePlaylist); // Returns -1 if there is no active playlist.
    Example2:
    plman.ActivePlaylist = 1; // Switches to 2nd playlist.
    */

    /**
     * 0 - Default
     * 1 - Repeat (Playlist)
     * 2 - Repeat (Track)
     * 3 - Random
     * 4 - Shuffle (tracks)
     * 5 - Shuffle (albums)
     * 6 - Shuffle (folders)
     *
     * @type {number}
     */
    this.PlaybackOrder = undefined; // (uint) (read, write)


    /** @type {number} */
    this.PlayingPlaylist = undefined; // (int) (read, write)
    /*
    Example:
    console.log(plman.PlayingPlaylist); // Returns -1 if there is no playing playlist
    */

    /** @type {number} */
    this.PlaylistCount = undefined; // (uint) (read)

    /** @type {IFbPlaylistRecyclerManager} */
    this.PlaylistRecyclerManager = undefined; // (IFbPlaylistRecyclerManager) (read)

    /**
     * @constructor
     */
    function PlaylistRecyclerManager() {

        /** @type {number} */
        this.Count = undefined; // (uint) (read)

        /**
         * @param {number} index
         * @return {string}
         */
        this.Name = function (index) {}; // (string) (read)

        /**
         * @param {number} index
         * @return {IFbMetadbHandleList}
         */
        this.Content = function (index) {}; // (IFbMetadbHandleList) (read)

        /**
         * @param {number} affectedItems
         */
        this.Purge = function (affectedItems) {}; // (void)
        // affectedItems: array like [1, 3, 5]

        /**
         * @param {number} index
         */
        this.Restore = function (index) {}; // (void)
    }

    // In all these methods, playlistIndex is the target playlist

    /**
     * This operation is asynchronous and may take some time to complete if it's a large array.
     *
     * @param {number} playlistIndex
     * @param {Array<string>} paths An array of files/URLs
     * @param {boolean=} [select=false]
     */
    this.AddLocations = function (playlistIndex, paths, select) {}; // (void) [, select]
    /*
    Example:
    plman.AddLocations(plman.ActivePlaylist, ["e:\\1.mp3"]);
    This operation is asynchronous and may take some time to complete if it's a large array.
    Any code in your script directly after this line will run immediately without waiting for
    the job to finish.
    If select is true, the active playlist will be set to the playlistIndex, the items will
    be selected and focus will be set to the first new item.
    */

    /**
     * @param {number} playlistIndex
     */
    this.ClearPlaylist = function (playlistIndex) {}; // (void)
    /*
    Example:
    plman.ClearPlaylist(plman.PlayingPlaylist);
    */

    /**
     * @param {number} playlistIndex
     */
    this.ClearPlaylistSelection = function (playlistIndex) {}; // (void)
    /*
    Example:
    plman.ClearPlaylistSelection(plman.ActivePlaylist);
    */

    /**
     * @param {number} playlistIndex
     * @param {string} name A name for the new Autplaylist
     * @param {string} query A valid query
     * @param {string=} [sort=''] Title formatting pattern.
     * @param {number=} [flags=0] Use 1 to force sort
     * @return {number} -1 on failure
     */
    this.CreateAutoPlaylist = function (playlistIndex, name, query, sort, flags) {}; // (int) [, sort][, flags]

    /**
     * @param {number} playlistIndex
     * @param {string} name
     * @return {number} -1 on failure.
     */
    this.CreatePlaylist = function (playlistIndex, name) {}; // (int)
    /*
    Example1:
    plman.CreatePlaylist(0, "");
    Creates a new playlist first in the list and it will be named "New playlist"
    Numbers will be appended to the end for each new un-named playlist
    Example2:
    plman.CreatePlaylist(plman.PlaylistCount, "my favourites");
    This will be added at the end of the current playlists.
    */

    /**
     * The duplicate playlist gets inserted directly after the source playlistIndex.
     * It only duplicates playlist content, not the properties of the playlist eg. Autoplaylist
     *
     * @param {number} playlistIndex
     * @param {string} name A name for the new playlist. If name is "", the name of the source playlist is used.
     * @return {number}
     */
    this.DuplicatePlaylist = function (playlistIndex, name) {}; // (int)

    /**
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     */
    this.EnsurePlaylistItemVisible = function (playlistIndex, playlistItemIndex) {}; // (void)

    /**
     * Starts playback by executing default doubleclick/enter action unless overridden by a lock to do something else.
     *
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     * @return {boolean} -1 on failure.
     */
    this.ExecutePlaylistDefaultAction = function (playlistIndex, playlistItemIndex) {}; // (boolean)

    /**
     * Returns playlistIndex of named playlist or creates new one if not found.
     * If a new playlist is created, the playlistIndex of that will be returned.
     *
     * @param {string} name
     * @param {boolean} unlocked If true, locked playlists are ignored when looking for existing playlists.
     *                           If false, the playlistIndex of any playlist with the matching name will be returned.
     * @return {number}
     */
    this.FindOrCreatePlaylist = function (name, unlocked) {}; // (int)

    /**
     * @param {string} name Case insensitive.
     * @return {number} playlistIndex or -1 on failure.
     */
    this.FindPlaylist = function (name) {}; // (int)

    /**
     * Retrieves playlist position of currently playing item.
     * On failure, the property {@link IFbPlayingItemLocation.IsValid} will be set to false.
     *
     * @return {IFbPlayingItemLocation}
     */
    this.GetPlayingItemLocation = function () {}; // (IFbPlayingItemLocation)

    /**
     * @constructor
     */
    function IFbPlayingItemLocation() {

        /** @type {boolean} */
        this.IsValid = undefined; // (boolean) (read)
        /** @type {number} */
        this.PlaylistIndex = undefined; // (int) (read)
        /** @type {number} */
        this.PlaylistItemIndex = undefined; // (int) (read)
    }

    /*
    Example:
    var playing_item_location = plman.GetPlayingItemLocation();
    if (playing_item_location.IsValid) {
        console.log(playing_item_location.PlaylistIndex);
        console.log(playing_item_location.PlaylistItemIndex);
    }
    */

    /**
     * @param {number} playlistIndex
     * @return {number} Returns -1 if nothing is selected
     */
    this.GetPlaylistFocusItemIndex = function (playlistIndex) {}; // (int)
    /*
    Example:
    var focus_item_index = plman.GetPlaylistFocusItemIndex(plman.ActivePlaylist); // 0 would be the first item
    */

    /**
     * @param {number} playlistIndex
     * @return {IFbMetadbHandleList}
     */
    this.GetPlaylistItems = function (playlistIndex) {}; // (IFbMetadbHandleList)
    /*
    Example:
    var handle_list = plman.GetPlaylistItems(plman.PlayingPlaylist);
    */

    /**
     * @param {number} playlistIndex
     * @return {string}
     */
    this.GetPlaylistName = function (playlistIndex) {}; // (string)
    /*
    Example:
    console.log(plman.GetPlaylistName(plman.ActivePlaylist));
    */

    /**
     * @param {number} playlistIndex
     * @return {IFbMetadbHandleList}
     */
    this.GetPlaylistSelectedItems = function (playlistIndex) {}; // (IFbMetadbHandleList)
    /*
    Example:
    var selected_items = plman.GetPlaylistSelectedItems(plman.ActivePlaylist);
    */

    /**
     * @param {number} playlistIndex
     * @param {number} base Position in playlist
     * @param {IFbMetadbHandleList} handle_list Items to insert
     * @param {boolean=} [select=false]
     * @return {number}
     */
    this.InsertPlaylistItems = function (playlistIndex, base, handle_list, select) {}; // (int) [, select]
    /*
    example1:
    var ap = plman.ActivePlaylist;
    plman.InsertPlaylistItems(ap, 0, fb.GetLibraryItems());
    Adds all library tracks to beginning of playlist.

    example2:
    var ap = plman.ActivePlaylist;
    plman.InsertPlaylistItems(ap, plman.PlaylistItemCount(ap), fb.GetLibraryItems());
    Adds all library tracks to end of playlist.
    */

    /**
     * Same as {@link plman.InsertPlaylistItems} except any duplicates contained in handle_list are removed.
     *
     * @param {number} playlistIndex
     * @param {number} base
     * @param {IFbMetadbHandleList} handle_list
     * @param {boolean=} [select=false]
     * @return {number}
     */
    this.InsertPlaylistItemsFilter = function (playlistIndex, base, handle_list, select) {}; // (int) select = false
    /*

    /**
     * @param {number} playlistIndex
     * @return {boolean}
     */
    this.IsAutoPlaylist = function (playlistIndex) {}; // (boolean)

    /**
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     * @return {boolean}
     */
    this.IsPlaylistItemSelected = function (playlistIndex, playlistItemIndex) {}; // (boolean)

    /**
     * @param {number} playlistIndex
     * @return {boolean}
     */
    this.IsPlaylistLocked = function (playlistIndex) {}; // (boolean)
    /*
    NOTE: This returns true if the playlist is an autoplaylist. To determine if a playlist is not an autoplaylist
    but locked with something like foo_utils or foo_playlist_attributes, do something like...

    if (!plman.IsAutoPlaylist(plman.ActivePlaylist) && plman.IsPlaylistLocked(plman.ActivePlaylist)) {
        blah();
    }
    */

    /**
     * @param {number} from
     * @param {number} to
     * @return {boolean}
     */
    this.MovePlaylist = function (from, to) {}; // (boolean)

    /**
     * @param {number} playlistIndex
     * @param {number} delta
     * @return {boolean}
     */
    this.MovePlaylistSelection = function (playlistIndex, delta) {}; // (boolean)
    /*
    Example:
    plman.MovePlaylistSelection(plman.ActivePlaylist, plman.PlaylistItemCount(plman.ActivePlaylist));
    Moves selected items to end of playlist.
    */

    /**
     * @param {number} playlistIndex
     * @return {number}
     */
    this.PlaylistItemCount = function (playlistIndex) {}; // (uint) (read)
    /*
    Example:
    console.log(plman.PlaylistItemCount(plman.PlayingPlaylist)); // 12
    */

    /**
     * Removes the specified playlist.
     * Note: if removing the active playlist, no playlist will be active after using this. You'll
     * need to set it manually or use plman.RemovePlaylistSwitch instead.
     *
     * @param {number} playlistIndex
     * @return {boolean}
     */
    this.RemovePlaylist = function (playlistIndex) {}; // (boolean)

    /**
     * @param {number} playlistIndex
     * @param {boolean=} [crop=false] If crop is true, then removes items that are NOT selected.
     */
    this.RemovePlaylistSelection = function (playlistIndex, crop) {}; // (void) [, crop]
    /*
    Example1:
    plman.RemovePlaylistSelection(plman.ActivePlaylist);
    Removes selected items from playlist.
    Example2:
    plman.RemovePlaylistSelection(plman.ActivePlaylist, true);
    Removes items that are NOT selected.
    */

    /**
     * Removes the specified playlist.
     * This automatically sets another playlist as active if removing the active playlist.
     *
     * @param {number} playlistIndex
     * @return {boolean}
     */
    this.RemovePlaylistSwitch = function (playlistIndex) {}; // (boolean)

    /**
     * @param {number} playlistIndex
     * @param {string} name
     * @return {boolean}
     */
    this.RenamePlaylist = function (playlistIndex, name) {}; // (boolean)

    /**
     * Workaround so you can use the Edit menu or run fb.RunMainMenuCommand("Edit/Something...")
     * when your panel has focus and a dedicated playlist viewer doesn't.
     */
    this.SetActivePlaylistContext = function () {}; // (void)
    /*
    Example:

    plman.SetActivePlaylistContext(); // once on startup

    function on_focus(is_focused) {
        if (is_focused) {
            plman.SetActivePlaylistContext(); // When the panel gets focus but not on every click
        }
    }
    */

    /**
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     */
    this.SetPlaylistFocusItem = function (playlistIndex, playlistItemIndex) {}; // (void)
    /*
    Example:
    plman.SetPlaylistFocusItem(plman.ActivePlaylist, 0);
    */

    /**
     * @param {number} playlistIndex
     * @param {IFbMetadbHandle} handle
     */
    this.SetPlaylistFocusItemByHandle = function (playlistIndex, handle) {}; // (void)
    /*
    Example:
    var ap = plman.ActivePlaylist;
    var handle = plman.GetPlaylistItems(ap)[1]; // 2nd item in playlist
    plman.SetPlaylistFocusItemByHandle(ap, handle);
    */

    /**
     * @param {number} playlistIndex
     * @param {Array<number>} affectedItems An array of item indexes.
     * @param {boolean} state
     */
    this.SetPlaylistSelection = function (playlistIndex, affectedItems, state) {}; // (void)
    /*
    Example:
    plman.SetPlaylistSelection(plman.ActivePlaylist, [0, 2, 4], true);
    Selects first, third and fifth tracks in playlist. This does not affect other selected items.
    */

    /**
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     * @param {boolean} state
     */
    this.SetPlaylistSelectionSingle = function (playlistIndex, playlistItemIndex, state) {}; // (void)
    /*
    Example1:
    plman.SetPlaylistSelectionSingle(plman.ActivePlaylist, 0, false);
    Deselects first playlist item. Only works when it is already selected!
    Example2:
    var ap = plman.ActivePlaylist;
    plman.SetPlaylistSelectionSingle(ap, plman.PlaylistItemCount(ap) - 1, true);
    Selects last item in playlist. This does not affect other selected items.
    */

    /**
     * Shows popup window letting you edit certain Autoplaylist properties.
     * Before using, check if your playlist is an Autoplaylist by using {@link plman.IsAutoPlaylist};
     *
     * @param {number} playlistIndex
     * @return {boolean}
     */
    this.ShowAutoPlaylistUI = function (playlistIndex) {}; // (boolean)
    /*
    Example:
    fb.ShowAutoPlaylistUI(plman.ActivePlaylist);
    */

    /**
     * @param {number} playlistIndex Index of playlist to alter.
     * @param {string} pattern Title formatting pattern to sort by. Set to "" to randomise the order of items.
     * @param {boolean=} [selected_items_only=false]
     * @return {boolean} true on success, false on failure (playlist locked etc).
     */
    this.SortByFormat = function (playlistIndex, pattern, selected_items_only) {}; // (boolean) [, selected_items_only]

    /**
     * @param {number} playlistIndex Index of playlist to alter.
     * @param {string} pattern Title formatting pattern to sort by.
     * @param {number=} [direction=1]
     *     1 - ascending
     *     -1 - descending
     * @return {boolean}
     */
    this.SortByFormatV2 = function (playlistIndex, pattern, direction) {}; // (boolean) [, direction]

    /**
     * @param {number=} [direction=1]
     *     1 - ascending
     *     -1 - descending
     */
    this.SortPlaylistsByName = function (direction) {}; //(void)

    /**
     * Call before using other plman methods that add/remove/reorder playlist items so a history will be available from the Edit menu.
     *
     * @param {number} playlistIndex
     */
    this.UndoBackup = function (playlistIndex) {}; // (void)

    /**
     * @param {IFbMetadbHandle} handle
     */
    this.AddItemToPlaybackQueue = function (handle) {}; // (void)

    /**
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     */
    this.AddPlaylistItemToPlaybackQueue = function (playlistIndex, playlistItemIndex) {}; // (void)

    /**
     * @param {IFbMetadbHandle} handle
     * @param {number} playlistIndex
     * @param {number} playlistItemIndex
     * @return {number} -1 on failure
     */
    this.FindPlaybackQueueItemIndex = function (handle, playlistIndex, playlistItemIndex) {}; // (int)

    this.FlushPlaybackQueue = function () {}; // (void)

    /**
     * @return {Array<IFbPlaybackQueueItem>}
     */
    this.GetPlaybackQueueContents = function () {}; // (Array)
    /*
    Example:
    var contents = plman.GetPlaybackQueueContents();
    if (contents.length) {
        // access properties of first item
        console.log(contents[0].PlaylistIndex, contents[0].PlaylistItemIndex);
    }
    */

    /**
     * @constructor
     */
    function IFbPlaybackQueueItem() {

        /** @type {IFbMetadbHandle} */
        this.Handle = undefined; // (IFbMetadbHandle) (read)

        /** @type {number} */
        this.PlaylistIndex = undefined; // (int) (read)

        /**
         * -1 if added not from a playlist
         *
         * @type {number}
         */
        this.PlaylistItemIndex = undefined; // (int) (read)
    }

    /*
     * @return {IFbMetadbHandleList}
     */
    this.GetPlaybackQueueHandles = function () {}; // ((IFbMetadbHandleList))
    /*
    var handles = plman.GetPlaybackQueueHandles();
    if (handles.Count > 0) {
        // use "Count" to determine if Playback Queue is active.
    }
    */

    /**
     * @param {number} index
     */
    this.RemoveItemFromPlaybackQueue = function (index) {}; // (void)

    /**
     * @param {Array<number>} affectedItems Array like [1, 3, 5]
     */
    this.RemoveItemsFromPlaybackQueue = function (affectedItems) {}; // (void)
}

var plman = new IFbPlaylistManager();

/**
 * @constructor
 */
function IJSConsole() {
    /** @param {...*} var_args */
    this.log = function (var_args) {}; // (void)
}

var console = new IJSConsole();

/**
 * @constructor
 */
function IJSUtils() {

    /**
     * New in v1.2.0. A 4 digit number corresponding to the version.
     * v1.2.0 -> 1200
     * v1.2.1 -> 1210
     *
     * @type {number} */
    this.Version = undefined; // (uint) (read)
    /*
    If you try and access this in older components where it doesn't exist, the script will
    crash so you can do a check like this.

    if (!("Version" in utils)) {
        fb.ShowPopupMessage("Current component version is less than v1.2.0. This script requires vX.X.X");
    } else {
        //check the actual version
    }
    */

    /**
     * @param {string} name
     * @param {boolean=} [is_dll=true] If true, method checks filename as well as the internal name.
     * @return {boolean}
     */
    this.CheckComponent = function (name, is_dll) {}; //(boolean)
    /*
    Example:
    console.log(utils.CheckComponent("foo_playcount", true));
    */

    /**
     * This only checks for fonts that are actually installed. It cannot detect fonts
     * loaded by foo_ui_hacks and always returns false. However, {@link gdi.Font} can use those fonts.
     *
     * @param {string} name Can be either in English or the localised name in your OS.
     * @return {boolean}
     */
    this.CheckFont = function (name) {}; // (boolean)

    /**
     * Spawns a windows popup dialog to let you choose a colour.
     *
     * @param {number} window_id {@link window.ID}
     * @param {number} default_colour
     * @return {number}
     */
    this.ColourPicker = function (window_id, default_colour) {}; // (int)
    /*
    Example:
    var colour = utils.ColourPicker(window.ID, RGB(255, 0, 0));
    See docs\Helper.js for RGB function.
    */

    /**
     * Creates a window with html page (rendered by IE engine).
     *
     * @param {string} code Html source code of the page
     * @param {object=} [options={}]
     * @param {number=} [options.width=400] Window width
     * @param {number=} [options.height=400] Window height
     * @param {string=} [options.title='foobar2000'] Window title
     * @param {*=} [options.data=undefined] Will be saved in window.stored_data object and can be accessed from JavaScript executed inside HTML window.
     *                                      This data is read-only and should not be modified.
     * @param {object=} [options.fn=undefined] Will be saved in window.stored_function object and can be accessed from JavaScript as well.
     *                                         This function can have up to 7 arguments and can be used as callback to pass some data back to the caller.
     */
    this.CreateHtmlWindow = function(code, options){};

    /**
     * @param {string} path
     * @param {string} mode
     *     "chardet" - Guess the charset of a file and return the codepage. It may not be accurate and returns 0 if an error occurred.
     *     "e" - If file path exists, return true.
     *     "s" - Retrieve file size, in bytes.
     *     "d" - If path is a directory, return true.
     *     "split" - Returns an Array.
     * @return {Array<string>}
     */
    this.FileTest = function (path, mode) {}; // (VARIANT)
    /*
    Example:
    var arr = utils.FileTest("D:\\Somedir\\Somefile.txt", "split");
    arr[0] <= "D:\\Somedir\\" (always includes backslash at the end)
    arr[1] <= "Somefile"
    arr[2] <= ".txt"
    */

    /**
     * @param {number} seconds
     * @return {string}
     */
    this.FormatDuration = function (seconds) {}; // (string)
    /*
    Example:
    console.log(utils.FormatDuration(plman.GetPlaylistItems(plman.ActivePlaylist).CalcTotalDuration())); // 1wk 1d 17:25:30
    */

    /**
     * @param {number} bytes
     * @return {string}
     */
    this.FormatFileSize = function (bytes) {}; // (string)
    /*
    Example:
    console.log(utils.FormatFileSize(plman.GetPlaylistItems(plman.ActivePlaylist).CalcTotalSize())); // 7.9 GB
    */

    /**
     * See samples\basic\GetAlbumArtAsync.txt
     *
     * @param {number} window_id {@link window.ID}
     * @param {IFbMetadbHandle} handle
     * @param {number=} [art_id=0] See Flags.js > AlbumArtId
     * @param {boolean=} [need_stub=true]
     * @param {boolean=} [only_embed=false]
     * @param {boolean=} [no_load=false]  If true, "image" parameter will be null in on_get_album_art_done callback.
     * @return {number}
     */
    this.GetAlbumArtAsync = function (window_id, handle, art_id, need_stub, only_embed, no_load) {}; // (uint) [, art_id][, need_stub][, only_embed][, no_load]

    /**
     * @param {string} rawpath
     * @param {number=} [art_id=0] See Flags.js > AlbumArtId
     * @return {IGdiBitmap}
     */
    this.GetAlbumArtEmbedded = function (rawpath, art_id) {}; // (IGdiBitmap) [, art_id]
    /*
    Example:
    var img = utils.GetAlbumArtEmbedded(fb.GetNowPlaying().RawPath, 0);
    */

    /**
     * See samples\basic\GetAlbumArtV2.txt
     *
     * @param {number} window_id
     * @param {IFbMetadbHandle} handle
     * @param {number=} [art_id=0] See Flags.js > AlbumArtId
     * @param {boolean=} [need_stub=true]
     * @return {IGdiBitmap}
     */
    this.GetAlbumArtV2 = function (handle, art_id, need_stub) {}; // (IGdiBitmap) [, art_id][, need_stub]

    /**
     * @param {number} index {@link http://msdn.microsoft.com/en-us/library/ms724371%28VS.85%29.aspx}
     * @return {number} 0 if failed
     */
    this.GetSysColor = function (index) {}; // (uint)
    /*
    Example:
    var splitter_colour = utils.GetSysColour(15);
    */

    /**
     * @param {number} index {@link http://msdn.microsoft.com/en-us/library/ms724385%28VS.85%29.aspx}
     * @return {number} 0 if failed
     */
    this.GetSystemMetrics = function (index) {}; // (int)

    /**
     * @param {string} pattern
     * @param {number=} [exc_mask=0x10] Default is FILE_ATTRIBUTE_DIRECTORY. See Flags.js > Used in utils.Glob()
     * @param {number=} [inc_mask=0xffffffff]
     * @return {Array<string>}
     */
    this.Glob = function (pattern, exc_mask, inc_mask) {}; // (Array) [, exc_mask][, inc_mask]
    /*
    Example:
    var arr = utils.Glob("C:\\*.*");
    */

    /**
     * @param {number} window_id
     * @param {string} prompt
     * @param {string} caption
     * @param {string=} [defaultval = '']
     * @param {boolean=} [error_on_cancel = false] If set to true, use try/catch like Example2.
     * @return {string}
     */
    this.InputBox = function(window_id, prompt, caption, defaultval, error_on_cancel) {}; // (string)
    /*
    Example1:
    var username = utils.InputBox(window.ID, "Enter your username", "Spider Monkey Panel", "");
    With "error_on_cancel" not set (or set to false), cancelling the dialog will return "defaultval".
    Example2:
    Using Example1, you can't tell if OK or Cancel was pressed if the return value is the same
    as "defaultval". If you need to know, set "error_on_cancel" to true which throws a script error
    when Cancel is pressed.
    try {
        var username = utils.InputBox(window.ID, "Enter your username", "Spider Monkey Panel", "", true);
        // OK was pressed.
    } catch(e) {
        // Dialog was closed by pressing Esc, Cancel or the Close button.
    }
    */

    /**
     * @param {number} vkey {@link http://msdn.microsoft.com/en-us/library/ms927178.aspx}. Some are defined in Flags.js > Used with utils.IsKeyPressed().
     * @return {boolean}
     */
    this.IsKeyPressed = function (vkey) {}; // (boolean)

    /**
     * @param {string} text
     * @param {string} lcid
     * @param {number} flags
     * @return {string}
     */
    this.MapString = function (text, lcid, flags) {}; // (string)

    /**
     * Using Microsoft MS-DOS wildcards match type. eg "*.txt", "abc?.tx?"
     *
     * @param {string} pattern
     * @param {string} str
     * @return {boolean}
     */
    this.PathWildcardMatch = function (pattern, str) {}; // (boolean)

    /**
     * @param {string} filename
     * @param {number=} [codepage=0] See Codepages.js. If codepage is 0, text file can be UTF16-BOM, UTF8-BOM or ANSI.
     * @return {string}
     */
    this.ReadTextFile = function (filename, codepage) {}; // (string) [,codepage]
    /*
    Example:
    var text = utils.ReadTextFile("E:\\some text file.txt");
    */

    /**
     * @param {string} filename
     * @param {string} section
     * @param {string} key
     * @param {string=} defaultval
     * @return {string}
     */
    this.ReadINI = function (filename, section, key, defaultval) {}; // (string) [, defaultval]
    /*
    An INI file should like this:

    [section]
    key=val

    This only returns up to 255 characters per value.
    Example:
    var username = utils.ReadINI("e:\\my_file.ini", "Last.fm", "username");
    */

    /**
     * @param {string} filename
     * @param {string} section
     * @param {string} key
     * @param {string} val
     * @return {boolean}
     */
    this.WriteINI = function (filename, section, key, val) {}; // (boolean)
    /*
    Example:
    utils.WriteINI("e:\\my_file.ini", "Last.fm", "username", "Bob");
    */

    /**
     * The parent folder must already exist.
     *
     * @param {string} filename
     * @param {string} content
     * @param {boolean=} [write_bom=true]
     * @return {boolean}
     */
    this.WriteTextFile = function (filename, content, write_bom) {}; //(boolean)
    /*
    Example:
    utils.WriteTextFile("z:\\1.txt", "test"); // write_bom missing but defaults to true, resulting file is UTF8-BOM
    utils.WriteTextFile("z:\\2.txt", "test", true); // resulting file is UTF8-BOM
    utils.WriteTextFile("z:\\3.txt", "test", false); // resulting file is UTF8 without BOM
    */
}

var utils = new IJSUtils();

/**
 * @constructor
 */
function Fb2kWindow() {
    /**
     * See Flags.js > With window.DlgCode
     *
     * @return {number}
     */
    this.DlgCode = function () {}; // (int) (read, write)
    /*
    Example:
    window.DlgCode(DLGC_WANTALLKEYS);
    */

    /** @type {number} */
    this.ID = undefined; // (read) (int)
    /*
        Required in fb.CheckClipboardContents, fb.GetClipboardContents, utils.ColourPicker, utils.GetAlbumArtAsync,
        utils.InputBox, utils.LoadImageAsync, etc.
    */

    /**
     * 0 - if using Columns UI
     * 1 - if using default UI.
     *
     * @type {number}
     */
    this.InstanceType = undefined; // (int)
    /*
    You need this to determine which GetFontXXX and GetColourXXX methods to use, assuming you want to support both interfaces.
    */

    /**
     * Depends on setting inside Spider Monkey Panel Configuration window. You generally use it to determine
     * whether or not to draw a background. Only useful within Panel Stack Splitter (Columns UI component)
     *
     * @type {boolean}
     */
    this.IsTransparent = undefined; // (boolean) (read)

    /** @type {boolean} */
    this.IsVisible = undefined; // (boolean) (read)

    /** @type {number} */
    this.Height = undefined; // (int) (read)

    /** @type {number} */
    this.MaxHeight = undefined; // (int) (read, write)

    /** @type {number} */
    this.MaxWidth = undefined; // (int) (read, write)

    /** @type {number} */
    this.MinHeight = undefined; // (int) (read, write)

    /** @type {number} */
    this.MinWidth = undefined; // (int) (read, write)
    // The previous 4 methods can be used to lock the panel size. Do not use if panels are contained within Panel Stack Splitter (Columns UI component).

    /**
     * Returns the author set in {@link window.DefinePanel}
     * If that isn't present, the GUID of the panel is returned.
     *
     * @type {string}
     */
    this.Name = undefined; // (string) (read)

    /** @type {number} */
    this.Width = undefined; // (int) (read)

    /**
     * @param {number} timerID
     */
    this.ClearTimeout = function (timerID) {}; // (void)

    /**
     * @param {number} timerID
     */
    this.ClearInterval = function (timerID) {}; // (void)

    /**
     * Setups the panel information and available features.
     * Can be called only once, so it's better to define it
     * directly in the panel Configure menu.
     *
     * @param {string} name Displayed panel name
     * @param {object=} [options={}]
     * @param {string=} [options.author=''] Script author
     * @param {string=} [options.version=''] Script version
     * @param {object=} [options.features=undefined] Additional script features
     * @param {boolean=} [options.features.drag_n_drop=false] Indicates if drag_n_drop functionality should be enabled
     */
    this.DefinePanel = function (name, options) {}; // (void)

    /**
     * @return {number}
     */
    this.SetInterval = function (func, delay) {}; // (uint)

    /**
     * See samples\basic\Timer.txt
     *
     * @return {number}
     */
    this.SetTimeout = function (func, delay) {}; // (uint)

    /**
     * See samples\basic\MainMenuManager All-In-One, samples\basic\Menu Sample.txt
     *
     * @return {IMenuObj}
     */
    this.CreatePopupMenu = function () {}; // (IMenuObj)

    /**
     * @constructor
     */
    function IMenuObj() {

        /**
         * @param {number} flags See Flags.js > Used in AppendMenuItem()
         * @param {number} item_id Integer greater than 0. Each menu item needs a unique id.
         * @param {string} text
         */
        this.AppendMenuItem = function (flags, item_id, text) {}; // (void)

        this.AppendMenuSeparator = function () {}; // (void)

        /**
         * @param {IMenuObj} parentMenu
         * @param {number} flags
         * @param {string} text
         */
        this.AppendTo = function (parentMenu, flags, text) {}; // (void)

        /**
         * @param {number} item_id
         * @param {boolean} check
         */
        this.CheckMenuItem = function (item_id, check) {}; // (void)

        /**
         * @param {number} first_item_id
         * @param {number} last_item_id
         * @param {number} selected_item_id
         */
        this.CheckMenuRadioItem = function (first_item_id, last_item_id, selected_item_id) {}; // (void)

        /**
         * @param {number} x
         * @param {number} y
         * @param {number=} [flags=0] See Flags.js > Used in TrackPopupMenu().
         * @return {number}
         */
        this.TrackPopupMenu = function (x, y, flags) {}; // (int) [, flags]
    }

    /**
     * See samples\basic\SimpleThemedButton.txt
     *
     * @param {Array<string>} class_list {@link http://msdn.microsoft.com/en-us/library/bb773210%28VS.85%29.aspx}
     * @return {IThemeManager}
     */
    this.CreateThemeManager = function (class_list) {}; // (IThemeManager)

    /**
     * @constructor
     */
    function IThemeManager() {
        /**
         * @param {IGdiGraphics} gr
         * @param {number} x
         * @param {number} y
         * @param {number} w
         * @param {number} h
         * @param {number=} [clip_x=0]
         * @param {number=} [clip_y=0]
         * @param {number=} [clip_w=0]
         * @param {number=} [clip_h=0]
         */
        this.DrawThemeBackground = function (gr, x, y, w, h, clip_x, clip_y, clip_w, clip_h) {}; // (void) [, clip_x][, clip_y][, clip_w][, clip_h]

        /**
         * @param {number} partid
         * @return {boolean}
         */
        this.IsThemePartDefined = function (partid) {}; // (boolean)

        /**
         * See {@link http://msdn.microsoft.com/en-us/library/bb773210%28VS.85%29.aspx}
         *
         * @param {number} partid
         * @param {number=} [stateid=0]
         */
        this.SetPartAndStateID = function (partid, stateid) {}; // (void)
    }

    /**
     * @param {string=} [font_name="Segoe UI"]
     * @param {number=} [font_size_px=12]
     * @param {number=} [font_style=0] See Flags.js > FontStyle
     * @return {IFbTooltip}
     */
    this.CreateTooltip = function (font_name, font_size_px, font_style) {}; // (IFbTooltip) [font_name][, font_size_px][, font_style]

    /**
     * @constructor
     */
    function IFbTooltip() {
        /*
        This will be used in the examples below.
        var tooltip = window.CreateTooltip();
        */

        /** @type {string} */
        this.Text = undefined; // (string) (read, write)
        /*
        Example:
        tooltip.Text = "Whoop";
        */

        /** @type {boolean} */
        this.TrackActivate = undefined; // (boolean) (write)

        this.Activate = function () {}; // (void)
        /*
        Only do this when text has changed otherwise it will flicker
        Example:
        var text = "...";
        if (tooltip.Text != text) {
            tooltip.Text = text;
            tooltip.Activate();
        }
        */

        this.Deactivate = function () {}; // (void)

        /**
         * @param {number} type
         * @return {number}
         */
        this.GetDelayTime = function (type) {}; // (int)

        /**
         * @param {number} type See Flags.js > Used in IFbTooltip.GetDelayTime() and IFbTooltip.SetDelayTime()
         * @param {number} time
         */
        this.SetDelayTime = function (type, time) {}; // (void)

        /**
         * Use if you want multi-line tooltips.
         * Use \n as a new line separator.
         *
         * @param {number} width
         */
        this.SetMaxWidth = function (width) {}; // (void)
        /*
        Example:
        tooltip.SetMaxWidth(800);
        tooltip.Text = "Line1\nLine2";
        */

        /**
         * Check x, y positions have changed from last time otherwise it will flicker
         *
         * @param {number} x
         * @param {number} y
         */
        this.TrackPosition = function (x, y) {}; // (void)
    }

    /**
     * @param {number} type
     * @param {string=} client_guid
     * @return {number}
     */
    this.GetColourCUI = function (type, client_guid) {}; // (uint) [, client_guid]

    /**
     * @param {number} type
     * @return {number}
     */
    this.GetColourDUI = function (type) {}; // (uint)

    /**
     * @param {number} type See Flags.js > Used in window.GetFontXXX()
     * @param {string=} client_guid See Flags.js > Used in GetFontCUI() as client_guid.
     * @return {IGdiFont} null if the component was unable to determine your font.
     */
    this.GetFontCUI = function (type, client_guid) {}; // (IGdiFont) [, client_guid]

    /**
     * @param {number} type See Flags.js > Used in window.GetFontXXX()
     * @return {IGdiFont} null if the component was unable to determine your font.
     */
    this.GetFontDUI = function (type) {}; // (IGdiFont)
    /*
    To avoid errors when trying to use the font or access its properties, you
    should use code something like this...

    var font = window.GetFontDUI(0);
    if (!font) {
        console.log("Unable to determine your default font. Using Segoe UI instead.");
        font = gdi.Font("Segoe UI", 12);
    }
    */

    /**
     * Listen for notifications in other panels using on_notify_data(name, info) {}
     *
     * @param {string} name
     * @param {*} info All variable/array/object types are supported
     */
    this.NotifyOthers = function (name, info) {}; // (void)

    /**
     * Reload panel.
     */
    this.Reload = function () {}; // (void)

    /**
     * @param {boolean=} [force=false]
     */
    this.Repaint = function (force) {}; // (void) [force]

    /**
     * Use this instead of window.Repaint on frequently updated areas
     * such as time, bitrate, seekbar, etc.
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {boolean=} [force=false]
     */
    this.RepaintRect = function (x, y, w, h, force) {}; // (void) [force]

    /**
     * @param {number} id See Flags.js > Used in window.SetCursor()
     */
    this.SetCursor = function (id) {}; // (void)
    /*
    This would usually be used inside the on_mouse_move callback. Use -1 if you want to hide the cursor.
    */

    /**
     * Get value of name from properties. If no value is present, defaultval will be stored and returned
     *
     * @param {string} name
     * @param {*=} defaultval
     * @return {*}
     */
    this.GetProperty = function (name, defaultval) {}; // (VARIANT) [, defaultval]

    /**
     * Set property value, if val is invalid/null, it is removed. Property values will be saved per panel instance and are
     * remembered between foobar2000 restarts.
     * @param {string} name
     * @param {*=} val
     */
    this.SetProperty = function (name, val) {}; // (void)

    /**
     * Show configuration window of current panel
     */
    this.ShowConfigure = function () {}; // (void)

    /**
     * Show properties window of current panel
     */
    this.ShowProperties = function () {}; // (void)
}

// Idea does not recognize this object if it has var =(
window = new Fb2kWindow();

/**
 * @constructor
 */
function IGdiFont() {
    /*
    This will be used in the examples below:
    var my_font = window.GetFontDUI(0);
    See Flags.js > FontTypeDUI
    */

    /** @type {number} */
    this.Height = undefined;//    (int)(read)
    /*
    Example:
    console.log(my_font.Height); // 15
    */

    /** @type {string} */
    this.Name = undefined;//    (string)(read)
    /*
    Example:
    console.log(my_font.Name); // Segoe UI
    */

    /** @type {float} */
    this.Size = undefined;//    (float)(read)
    /*
    Example:
    console.log(my_font.Size); // 12
    */

    /** @type {number} */
    this.Style = undefined;//    (int)(read)
    /*
    Example:
    console.log(my_font.Style);
    See Flags.js > FontStyle
    */
}

/**
 * @constructor
 */
function IGdiBitmap() {

    /** @type {number} */
    this.Height = undefined;// (int) (read)

    /** @type {number} */
    this.Width = undefined;// (int) (read)

    /**
     * @param {number} alpha Valid values 0-255.
     * @return {IGdiBitmap}
     */
    this.ApplyAlpha = function (alpha) {}; // (IGdiBitmap)

    /**
     * Changes will be saved in the current bitmap. See samples\basic\Apply Mask.txt
     *
     * @param {IGdiBitmap} img
     * @return {boolean}
     */
    this.ApplyMask = function (img) {}; // (boolean)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @return {IGdiBitmap}
     */
    this.Clone = function (x, y, w, h) {}; // (IGdiBitmap)

    /**
     * Create a DDB bitmap from IGdiBitmap, which is used in {@link gdi.GdiDrawBitmap}
     *
     * @return {IGdiRawBitmap}
     */
    this.CreateRawBitmap = function () {}; // (IGdiRawBitmap)

    /**
     * @constructor
     */
    function IGdiRawBitmap() {

        /** @type {number} */
        this.Width = undefined; // (int) (read)

        /** @type {number} */
        this.Height = undefined; // (int) (read)
    }

    /**
     * @param {number} max_count
     * @return {Array<number>}
     */
    this.GetColourScheme = function (max_count) {}; // (Array)

    /**
     * Returns a JSON array in string form so you need to use JSON.parse() on the result.
     * Each entry in the array is an object which contains colour and frequency values.
     * Uses a different method for calculating colours than GetColourScheme.
     * Image is automatically resized during processing for performance reasons so there's no
     * need to resize before calling the method.
     *
     * @param {number} max_count
     * @return {string}
     */
    this.GetColourSchemeJSON = function (max_count) {}; // (string)
    /*
    Example:
    img = ... // use utils.GetAlbumArtV2 / gdi.Image / etc
    colours = JSON.parse(img.GetColourSchemeJSON(5));
    console.log(colours[0].col); // -4194304
    console.log(colours[0].freq); // 0.34
    console.log(toRGB(colours[0].col)); // [192, 0, 0]

    See docs\Helpers.js for "toRGB" function.
    */

    /**
     * @return {IGdiGraphics}
     */
    this.GetGraphics = function () {};
    // Don't forget to use ReleaseGraphics() after operations on IGdiGraphics interface is done.

    /**
     * @param {IGdiGraphics} gr
     * @return {IGdiGraphics}
     */
    this.ReleaseGraphics = function (gr) {}; // (IGdiGraphics)

    /**
     * @param {number} w
     * @param {number} h
     * @param {number=} [mode=0] See Flags.js > InterpolationMode
     * @return{IGdiBitmap}
     */
    this.Resize = function (w, h, mode) {}; // (IGdiBitmap) [, mode]

    /**
     * @param {number} mode See Flags.js > RotateFlipType
     */
    this.RotateFlip = function (mode) {}; // (void)

    /**
     * @param {string} path Full path including file extension. The parent folder must already exist.
     * @param {string=} [format='image/png']
     *      "image/png" (default if omitted)
     *      "image/bmp"
     *      "image/jpeg"
     *      "image/gif"
     *      "image/tiff"
     * @return {boolean}
     */
    this.SaveAs = function (path, format) {}; // (boolean) [, format]
    /*
    Example:
    var img = utils.GetAlbumArtEmbedded(fb.GetFocusItem().RawPath, 0);
    if (img)
        img.SaveAs("D:\\export.jpg", "image/jpeg");
    */

    /**
     * @param {number} radius Valid values 2-254. See samples\basic\StackBlur (image).txt, samples\basic\StackBlur (text).txt
     */
    this.StackBlur = function (radius) {}; // (void)
}

/**
 * @constructor
 */
function IGdiGraphics() {
    /*
    Typically used inside on_paint(gr)
    There are many different ways to get colours.
    Use window.GetColourDUI/window.GetColourCUI,
    RGB function from Helpers.js, utils.ColourPicker,
    etc.
    */

    /**
     * Calculates text height for {@link IGdiGraphics.GdiDrawText}.
     *
     * @param {string} str
     * @param {IGdiFont} font
     * @return {number}
     */
    this.CalcTextHeight = function (str, font) {}; // (uint)
    // This will only calulate the text height of one line.

    /**
     * Calculates text width for {@link IGdiGraphics.GdiDrawText}.
     *
     * @param {string} str
     * @param {IGdiFont} font
     * @return {number}
     */
    this.CalcTextWidth = function (str, font) {}; // (uint)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} line_width
     * @param {number} colour
     */
    this.DrawEllipse = function (x, y, w, h, line_width, colour) {}; // (void)

    /**
     * @param {IGdiBitmap} img
     * @param {number} dstX
     * @param {number} dstY
     * @param {number} dstW
     * @param {number} dstH
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} srcW
     * @param {number} srcH
     * @param {number=} [angle=0]
     * @param {number=} [alpha=255] Valid values 0-255.
     */
    this.DrawImage = function (img, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, angle, alpha) {}; // (void) [, angle][, alpha]

    /**
     * @param {number} x1
     * @param {number} y1
     * @param {number} x2
     * @param {number} y2
     * @param {number} line_width
     * @param {number} colour
     */
    this.DrawLine = function (x1, y1, x2, y2, line_width, colour) {}; // (void)

    /**
     * @param {number} colour
     * @param {number} line_width
     * @param {Array<Array<number>>} points
     */
    this.DrawPolygon = function (colour, line_width, points) {}; // (void)

    /**
     * Should be only used when {@link IGdiGraphics.GdiDrawText} is not applicable.
     *
     * @param {string} str
     * @param {IGdiFont} font
     * @param {number} colour
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number=} [flags=0] See Flags.js > StringFormatFlags
     */
    this.DrawString = function (str, font, colour, x, y, w, h, flags) {}; // (void) [, flags]

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} line_width
     * @param {number} colour
     */
    this.DrawRect = function (x, y, w, h, line_width, colour) {}; // (void)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} arc_width
     * @param {number} arc_height
     * @param {number} line_width
     * @param {number} colour
     */
    this.DrawRoundRect = function (x, y, w, h, arc_width, arc_height, line_width, colour) {}; // (void)

    /**
     * @param {string} str
     * @param {IGdiFont}
     * @param {number} max_width
     * @return {Array<Array>}
     */
    this.EstimateLineWrap = function (str, font, max_width) {}; // (Array)
    /*
    index | meaning
    [0] text line 1
    [1] width of text line 1 (in pixel)
    [2] text line 2
    [3] width of text line 2 (in pixel)
    ...
    [2n + 2] text line n
    [2n + 3] width of text line n (px)
    */

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} colour
     */
    this.FillEllipse = function (x, y, w, h, colour) {}; // (void)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} angle
     * @param {number} colour1
     * @param {number} colour2
     * @param {float} [focus=1.0] Specify where the centred colour will be at its highest intensity. Valid values between 0 and 1.
     */
    this.FillGradRect = function (x, y, w, h, angle, colour1, colour2, focus) {}; // (void) [, focus]
    /*
    NOTE: This may appear buggy depending on rectangle size. The easiest fix is
    to adjust the "angle" by a degree or two.
    */

    /**
     * @param {number} colour
     * @param {number} fillmode 0 alternate, 1 winding.
     * @param {Array<Array<number>>} points
     */
    this.FillPolygon = function (colour, fillmode, points) {}; // (void)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} arc_width
     * @param {number} arc_height
     * @param {number} colour
     */
    this.FillRoundRect = function (x, y, w, h, arc_width, arc_height, colour) {}; // (void)

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number} colour
     */
    this.FillSolidRect = function (x, y, w, h, colour) {}; // (void)

    /**
     * @param {IGdiRawBitmap} img
     * @param {number} dstX
     * @param {number} dstY
     * @param {number} dstW
     * @param {number} dstH
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} srcW
     * @param {number} srcH
     * @param {number=} [alpha=255] Valid values 0-255.
     */
    this.GdiAlphaBlend = function (img, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, alpha) {}; // (void) [, alpha]

    /**
     * Always faster than {IGdiGraphics.DrawImage}, does not support alpha channel.
     *
     * @param {IGdiRawBitmap} img
     * @param {number} dstX
     * @param {number} dstY
     * @param {number} dstW
     * @param {number} dstH
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} srcW
     * @param {number} srcH
     */
    this.GdiDrawBitmap = function (img, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH) {}; // (void)

    /**
     * Provides faster and better rendering than {@link IGdiGraphics.DrawString}.
     *
     * Do not use this to draw text on transparent background or
     * with IGdiGraphics other than the one passed in on_paint callback:
     * this will result in visual artifacts caused by ClearType hinting.
     * Use {@link IGdiGraphics.DrawString} instead in such cases.
     *
     * To calculate text dimensions use {@link IGdiGraphics.CalcTextHeight}, {@link IGdiGraphics.CalcTextWidth} or DT_CALCRECT flag.
     *
     * @param {string} str
     * @param {IGdiFont} font
     * @param {number} colour
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number=} [format=0] See Flags.js > DT_*
     * @return {Array<number>}
     */
    this.GdiDrawText = function (str, font, colour, x, y, w, h, format) {}; // (Array) [, format]
    /*
    index | meaning
    [0] left   (DT_CALCRECT)
    [1] top    (DT_CALCRECT)
    [2] right  (DT_CALCRECT)
    [3] bottom (DT_CALCRECT)
    [4] characters drawn
    */

    /**
     * Calculates text dimensions for {@link IGdiGraphics.DrawString}.
     *
     * @param {string} str
     * @param {IGdiFont} font
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     * @param {number=} [flags=0] See Flags.js > StringFormatFlags
     * @return {IMeasureStringInfo}
     */
    this.MeasureString = function (str, font, x, y, w, h, flags) {}; // (IMeasureStringInfo) [, flags]

    /**
     * @constructor
     */
    function IMeasureStringInfo() {

        /** @type {number} */
        this.chars = undefined; // (int) (read)

        /** @type {float} */
        this.Height = undefined; // (float) (read)

        /** @type {number} */
        this.lines = undefined; // (int) (read)

        /** @type {float} */
        this.x = undefined; // (float) (read)

        /** @type {float} */
        this.y = undefined; // (float) (read)

        /** @type {float} */
        this.Width = undefined; // (float) (read)

        /*
        Example:
        include(fb.ComponentPath + 'docs\\Flags.js');
        include(fb.ComponentPath + 'docs\\Helpers.js');        

        var sf = StringFormat(StringAlignment.Near, StringAlignment.Near);
        var text = utils.ReadTextFile("z:\\info.txt");
        var font = window.GetFontDUI(0);

        function on_paint(gr) {
            gr.DrawString(text, font, RGB(255, 0, 0), 0, 0, window.Width, window.Height, sf);
            var temp = gr.MeasureString(text, font, 0, 0, window.Width, 10000, sf);
            // If we want to calculate height, we must set the height to be far larger than what
            // the text could possibly be.

            console.log(temp.Height); // 2761.2421875 // far larger than my panel height!
            console.log(temp.Chars); // 7967
        }
        */
    }

    /**
     * See Flags.js > InterpolationMode
     *
     * @param {number=} [mode=0]
     */
    this.SetInterpolationMode = function (mode) {}; // (void)

    /**
     * @param {number=} [mode=0] See Flags.js > SmoothingMode
     */
    this.SetSmoothingMode = function (mode) {}; // (void)

    /**
     * @param {number=} [mode=0] See Flags.js > TextRenderingHint
     */
    this.SetTextRenderingHint = function (mode) {}; // (void)
}

/**
 * @constructor
 */
function IFbMetadbHandle() {
    // This will be used in the examples below:
    // var handle = fb.GetFocusItem();

    /** @type {string} */
    this.Path = undefined; // (string) (read)
    /*
    Example:
    console.log(handle.Path); // D:\SomeSong.flac
    */

    /** @type {string} */
    this.RawPath = undefined; // (string) (read)
    /*
    Example:
    console.log(handle.RawPath); // file://D:\SomeSong.flac
    */

    /** @type {number} */
    this.SubSong = undefined; // (int) (read)

    /**
     * Requires a system with IE9 or later to work properly.
     *
     * @type {number}
     */
    this.FileSize = undefined; // (LONGLONG) (read)

    /** @type {float} */
    this.Length = undefined; // (double) (read)

    // See {@link https://github.com/marc2k3/foo_jscript_panel/wiki/Playback-Stats}

    /**
     * @param {number} playcount Use 0 to clear
     */
    this.SetPlayCount = function (playcount) {}; // (void)

    /**
     * @param {number} loved Use 0 to clear
     */
    this.SetLoved = function (loved) {}; // (void)

    /**
     * @param {string} first_played Use "" to clear
     */
    this.SetFirstPlayed = function (first_played) {}; // (void)

    /**
     * @param {string} last_played Use "" to clear
     */
    this.SetLastPlayed = function (last_played) {}; // (void)

    /**
     * @param {number} rating Use 0 to clear
     */
    this.SetRating = function (rating) {}; // (void)

    this.ClearStats = function () {}; // (void)

    this.RefreshStats = function () {}; // (void)

    /**
     * Compare two IFbMetadbHandle instances, pointer only.
     * If you want to compare them physically, use the "RawPath" property.
     *
     * @param {IFbMetadbHandle} handle
     * @return {boolean}
     */
    this.Compare = function (handle) {}; // (boolean)
    /*
    Example:
    handle.Compare(handle2);
    */

    /**
     * @return {IFbFileInfo}
     */
    this.GetFileInfo = function () {}; // (IFbFileInfo)

    /**
     * @constructor
     */
    function IFbFileInfo() {
        /*
        This will be used in the examples below:
        var handle = fb.GetFocusItem();
        var f = handle.GetFileInfo();
        */

        /** @type {number} */
        this.MetaCount = undefined; // (read)
        /*
        Example:
        console.log(f.MetaCount); // 11
        */

        /** @type {number} */
        this.InfoCount = undefined; // (read)
        /*
        Example:
        console.log(f.InfoCount); // 9
        */

        /**
         * @param {string} name
         * @return {number} -1 on failure
         */
        this.InfoFind = function (name) {}; //

        /**
         * @param {number} idx
         * @return {string}
         */
        this.InfoName = function (idx) {}; //

        /**
         * @param {number} idx
         * @return {string}
         */
        this.InfoValue = function (idx) {}; //

        /**
         * @param {string} name
         * @return {number} -1 on failure
         */
        this.MetaFind = function (name) {}; //

        /**
         * @param {number} idx
         * @return {string}
         */
        this.MetaName = function (idx) {}; //
        /*
        The case of the tag name returned can be different depending on tag type
        so using toLowerCase() or toUpperCase() on the result is recommended

        Example:
        for (var i = 0; i < f.MetaCount; i++) {
            console.log(f.MetaName(i).toUpperCase());
        }
        */

        /**
         * @param {number} idx
         * @param {number} vidx
         * @return {string}
         */
        this.MetaValue = function (idx, vidx) {}; //

        /**
         * @param {number} idx
         * @return {number}
         */
        this.MetaValueCount = function (idx) {}; //
    }
}

/**
 * Items can be accessed with array accessor, e.g. handle_list[i] 
 *
 * @constructor
 */
function IFbMetadbHandleList() {
    /*
    This will be used in the examples below:
    var handle_list = plman.GetPlaylistItems(plman.ActivePlaylist);
    If you want an empty handle list, do this...
    var handle_list = fb.CreateHandleList();
    */

    /** @type {number} */
    this.Count = undefined; // (uint) (read)
    /*
    Example:
    console.log(handle_list.Count);
    */

    /**
     * @param {IFbMetadbHandle} handle
     * @return {number}
     */
    this.Add = function (handle) {}; // (uint)
    /*
    Example:
    handle_list.Add(fb.GetNowPlaying());
    */

    /**
     * @param {IFbMetadbHandleList} handle
     */
    this.AddRange = function (handle_list) {}; // (void)
    /*
    Example:
    handle_list.AddRange(fb.GetLibraryItems());
    */

    /**
     * Note you cannot perform embedded art operations on the currently playing track - it will silently fail.
     * This is a limitation of the foobar2000 SDK. You may consider detecting if your handle list
     * contains the playing track and stopping playback, performing this action and then restarting
     * playback.
     * Any other errors such as invalid path, corrupt image, target file type not supporting
     * embedded art, etc should all silently fail. A progress dialog will be shown for larger file
     * selections. Any existing artwork of the specified type will be overwritten - there is no need to
     * remove it first.
     * 
     * @param {IFbMetadbHandleList} image_path path to an existing image
     * @param {number=} [art_id=0] See Flags.js > AlbumArtId
     */
    this.AttachImage(image_path, art_id) //(void)
    /*
    Example1:
    var handle_list = plman.GetPlaylistItems(plman.ActivePlaylist);
    if (handle_list.Count > 0) {
        var img_path = "C:\\path\\to\\image.jpg";
        handle_list.AttachImage(img_path, 0);
    }
    handle_list.Dispose();
    Example2:
    // since there is no handle method, do this for a single item
    var handle_list = fb.CreateHandleList();
    handle_list.Add(fb.GetFocusItem());
    var img_path = "C:\\path\\to\\image.jpg";
    handle_list.AttachImage(img_path, 0);
    handle_list.Dispose();
    */

    /**
     * Faster than Find()
     *
     * @param {IFbMetadbHandle} handle Must be sorted.
     * @return {number} -1 on failure.
     */
    this.BSearch = function (handle) {}; // (uint)

    /**
     * @return {float|number} total in seconds. For display purposes, consider using {@link utils.FormatDuration} on the result.
     */
    this.CalcTotalDuration = function () {}; // (double)

    /**
     * Requires a system with IE9 or later to work properly.
     * Returns total in bytes. For display purposes, consider using utils.FormatFileSize() on the result.
     *
     * @return {number}
     */
    this.CalcTotalSize = function () {}; // (LONGLONG)

    /**
     * @return {IFbMetadbHandleList}
     */
    this.Clone = function () {}; // (IFbMetadbHandleList)
    /*
    Example:
    var handle_list2 = handle_list.Clone();
    */

    /**
     * Should be faster than looping a IFbMetadbHandleList handle list.
     *
     * @return {Array<IFbMetadbHandle>}
     */
    this.Convert = function () {}; // (Array)
    /*
    It has been noticed by other users that looping through an array of handles rather
    than a handle list may be faster under some circumstances.

    Example:
    var playlist_items_array = plman.GetPlaylistItems(plman.ActivePlaylist).Convert();
    for (var i = 0; i < playlist_items_array.length; i++) {
        // do something with playlist_items_array[i] which is your handle
    }
    */

    /**
     * Note: If sorted, use {@link IFbMetadbHandleList.BSearch} instead
     *
     * @param {IFbMetadbHandle} handle
     * @return {number} -1 on failure
     */
    this.Find = function (handle) {}; // (int)

    /**
     * See {@link fb.GetLibraryRelativePath}.
     *
     * This should be faster than looping a handle list manually and using the aforementioned method.
     *
     * @return {Array<string>}
     */
    this.GetLibraryRelativePaths = function () {}; //(Array)
    /*
    Example:
    var handle_list = fb.GetLibraryItems();
    handle_list.OrderByRelativePath();
    var relative_paths = handle_list.GetLibraryRelativePaths();
    */

    /**
     * @param {number} index
     * @param {IFbMetadbHandle} handle
     * @return {number}
     */
    this.Insert = function (index, handle) {}; // (int)
    /*
    Example:
    handle_list.Insert(handle_list.Count, fb.GetNowPlaying());
    This inserts at the end of the handle list.
    */

    /**
     * @param {number} index
     * @param {IFbMetadbHandleList} handle
     * @return {number}
     */
    this.InsertRange = function (index, handle_list) {}; // (int)

    /**
     * @param {IFbMetadbHandleList} handle Must be sorted.
     */
    this.MakeDifference = function (handle_list) {}; // (void)
    /*
    Example:
    var one = plman.GetPlaylistItems(0);
    one.Sort();

    var two = plman.GetPlaylistItems(1);
    two.Sort();

    one.MakeDifference(two);
    // "one" now only contains handles that were unique to "one". Anything that also existed in "two" will have been removed.
    */

    /**
     * @param {IFbMetadbHandleList} handle Must be sorted.
     */
    this.MakeIntersection = function (handle_list) {}; // (void)
    /*
    Example:
    var one = plman.GetPlaylistItems(0);
    one.Sort();

    var two = plman.GetPlaylistItems(1);
    two.Sort();

    one.MakeIntersection(two);
    // "one" now only contains handles that were in BOTH "one" AND "two"
    */

    /**
     * @param {IFbMetadbHandleList} handle Must be sorted.
     */
    this.MakeUnion = function (handle_list) {}; // (void)
    /*
    Example:
    var one = plman.GetPlaylistItems(0);
    one.Sort();

    var two = plman.GetPlaylistItems(1);
    two.Sort();

    one.MakeUnion(two);
    // "one" now contains all handles from "one" AND "two" with any duplicates removed
    */

    /**
     * @param {IFbTitleFormat} tfo An instance of IFbTitleFormat.
     * @param {number} direction > 0 - ascending.
     */
    this.OrderByFormat = function (tfo, direction) {}; // (void)
    /*
    Example:
    var handle_list = fb.GetLibraryItems();
    var tfo = fb.TitleFormat("%album artist%|%date%|%album%|%discnumber%|%tracknumber%");
    handle_list.OrderByFormat(tfo, 1);
    */

    this.OrderByPath = function () {}; // (void)

    // This method should only be used on a handle list containing items that are monitored as part of the Media Library.
    this.OrderByRelativePath = function () {}; // (void)

    /**
     * See {@link https://github.com/marc2k3/foo_jscript_panel/wiki/Playback-Stats}
     */
    this.RefreshStats = function () {}; // (void)

    /**
     * @param {IFbMetadbHandle} handle
     */
    this.Remove = function (handle) {}; // (void)

    this.RemoveAll = function () {}; // (void)

    /**
     * See {@link IFbMetadbHandleList.AttachImage()} for more info as there are some limitaions.
     * 
     * @param {number=} [art_id=0] See Flags.js > AlbumArtId
     */
    this.RemoveAttachedImage(art_id); // (void)

    /**
     * @param {number} idx
     */
    this.RemoveById = function (idx) {}; // (void)
    /*
    Example:
    handle_list.RemoveById(0);
    */

    /**
     * @param {number} from
     * @param {number} num
     */
    this.RemoveRange = function (from, num) {}; // (void)
    /*
    Example:
    handle_list.RemoveRange(10, 20);
    */

    /**
     * Remove duplicates and optimise for other handle list operations
     */
    this.Sort = function () {}; // (void)


    /**
     * See {@link https://github.com/marc2k3/foo_jscript_panel/wiki/Breaking-Changes#v130}
     *
     * @param {string} str
     */
    this.UpdateFileInfoFromJSON = function (str) {}; // (void)
}

/**
 * See {@link https://github.com/marc2k3/foo_jscript_panel/wiki/Drag-and-Drop}
 * @constructor
 */
function IDropTargetAction() {

    /** @type {number} */
    this.Base = undefined; // (write)

    /**
     * See {@link https://msdn.microsoft.com/en-us/library/windows/desktop/ms693457.aspx}
     *
     * @type {number}
     */
    this.Effect = undefined //(read, write)

    /**
     * Active playlist.
     * -1 by default.
     *
     * @type {number}
     */
    this.Playlist = undefined; // (write)

    /** @type {boolean} */
    this.ToSelect = undefined; // (boolean) (write)
}
