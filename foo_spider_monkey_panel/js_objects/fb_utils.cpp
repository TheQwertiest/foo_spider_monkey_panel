#include <stdafx.h>
#include "fb_utils.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_ui_selection_holder.h>
#include <js_objects/main_menu_manager.h>
#include <js_objects/context_menu_manager.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_profiler.h>
#include <js_objects/fb_title_format.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <drop_source_impl.h>
#include <stats.h>
#include <popup_msg.h>


namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOp<JsFbUtils>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbUtils",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, AcquireUiSelectionHolder )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, AddDirectory )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, AddFiles )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CheckClipboardContents )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, ClearPlaylist )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CopyHandleListToClipboard )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CreateContextMenuManager )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CreateHandleList )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CreateMainMenuManager )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, CreateProfiler )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, DoDragDrop )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Exit )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetClipboardContents )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetDSPPresets )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetFocusItem )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetLibraryItems )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetLibraryRelativePath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetNowPlaying )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetOutputDevices )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetQueryItems )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetSelection )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetSelections )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, GetSelectionType )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, IsLibraryEnabled )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, IsMainMenuCommandChecked )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, IsMetadbInMediaLibrary )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, LoadPlaylist )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Next )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Pause )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Play )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, PlayOrPause )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Prev )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Random )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, RunContextCommand )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, RunContextCommandWithMetadb )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, RunMainMenuCommand )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, SaveIndex )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, SavePlaylist )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, SetDSPPreset )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, SetOutputDevice )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, ShowConsole )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, ShowLibrarySearchUI )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, ShowPopupMessage )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, ShowPreferences )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, Stop )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, TitleFormat )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, VolumeDown )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, VolumeMute )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, VolumeUp )


const JSFunctionSpec jsFunctions[] = {

    JS_FN( "AcquireUiSelectionHolder", AcquireUiSelectionHolder, 0, DefaultPropsFlags() ),
    JS_FN( "AddDirectory", AddDirectory, 0, DefaultPropsFlags() ),
    JS_FN( "AddFiles", AddFiles, 0, DefaultPropsFlags() ),
    JS_FN( "CheckClipboardContents", CheckClipboardContents, 0, DefaultPropsFlags() ),
    JS_FN( "ClearPlaylist", ClearPlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "CopyHandleListToClipboard", CopyHandleListToClipboard, 0, DefaultPropsFlags() ),
    JS_FN( "CreateContextMenuManager", CreateContextMenuManager, 0, DefaultPropsFlags() ),
    JS_FN( "CreateHandleList", CreateHandleList, 0, DefaultPropsFlags() ),
    JS_FN( "CreateMainMenuManager", CreateMainMenuManager, 0, DefaultPropsFlags() ),
    JS_FN( "CreateProfiler", CreateProfiler, 0, DefaultPropsFlags() ),
    JS_FN( "DoDragDrop", DoDragDrop, 0, DefaultPropsFlags() ),
    JS_FN( "Exit", Exit, 0, DefaultPropsFlags() ),
    JS_FN( "GetClipboardContents", GetClipboardContents, 0, DefaultPropsFlags() ),
    JS_FN( "GetDSPPresets", GetDSPPresets, 0, DefaultPropsFlags() ),
    JS_FN( "GetFocusItem", GetFocusItem, 0, DefaultPropsFlags() ),
    JS_FN( "GetLibraryItems", GetLibraryItems, 0, DefaultPropsFlags() ),
    JS_FN( "GetLibraryRelativePath", GetLibraryRelativePath, 0, DefaultPropsFlags() ),
    JS_FN( "GetNowPlaying", GetNowPlaying, 0, DefaultPropsFlags() ),
    JS_FN( "GetOutputDevices", GetOutputDevices, 0, DefaultPropsFlags() ),
    JS_FN( "GetQueryItems", GetQueryItems, 0, DefaultPropsFlags() ),
    JS_FN( "GetSelection", GetSelection, 0, DefaultPropsFlags() ),
    JS_FN( "GetSelections", GetSelections, 0, DefaultPropsFlags() ),
    JS_FN( "GetSelectionType", GetSelectionType, 0, DefaultPropsFlags() ),
    JS_FN( "IsLibraryEnabled", IsLibraryEnabled, 0, DefaultPropsFlags() ),
    JS_FN( "IsMainMenuCommandChecked", IsMainMenuCommandChecked, 0, DefaultPropsFlags() ),
    JS_FN( "IsMetadbInMediaLibrary", IsMetadbInMediaLibrary, 0, DefaultPropsFlags() ),
    JS_FN( "LoadPlaylist", LoadPlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "Next", Next, 0, DefaultPropsFlags() ),
    JS_FN( "Pause", Pause, 0, DefaultPropsFlags() ),
    JS_FN( "Play", Play, 0, DefaultPropsFlags() ),
    JS_FN( "PlayOrPause", PlayOrPause, 0, DefaultPropsFlags() ),
    JS_FN( "Prev", Prev, 0, DefaultPropsFlags() ),
    JS_FN( "Random", Random, 0, DefaultPropsFlags() ),
    JS_FN( "RunContextCommand", RunContextCommand, 0, DefaultPropsFlags() ),
    JS_FN( "RunContextCommandWithMetadb", RunContextCommandWithMetadb, 0, DefaultPropsFlags() ),
    JS_FN( "RunMainMenuCommand", RunMainMenuCommand, 0, DefaultPropsFlags() ),
    JS_FN( "SaveIndex", SaveIndex, 0, DefaultPropsFlags() ),
    JS_FN( "SavePlaylist", SavePlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "SetDSPPreset", SetDSPPreset, 0, DefaultPropsFlags() ),
    JS_FN( "SetOutputDevice", SetOutputDevice, 0, DefaultPropsFlags() ),
    JS_FN( "ShowConsole", ShowConsole, 0, DefaultPropsFlags() ),
    JS_FN( "ShowLibrarySearchUI", ShowLibrarySearchUI, 0, DefaultPropsFlags() ),
    JS_FN( "ShowPopupMessage", ShowPopupMessage, 0, DefaultPropsFlags() ),
    JS_FN( "ShowPreferences", ShowPreferences, 0, DefaultPropsFlags() ),
    JS_FN( "Stop", Stop, 0, DefaultPropsFlags() ),
    JS_FN( "TitleFormat", TitleFormat, 0, DefaultPropsFlags() ),
    JS_FN( "VolumeDown", VolumeDown, 0, DefaultPropsFlags() ),
    JS_FN( "VolumeMute", VolumeMute, 0, DefaultPropsFlags() ),
    JS_FN( "VolumeUp", VolumeUp, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_AlwaysOnTop )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_ComponentPath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_CursorFollowPlayback )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_FoobarPath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_IsPaused )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_IsPlaying )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_PlaybackFollowCursor )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_PlaybackLength )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_PlaybackTime )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_ProfilePath )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_ReplaygainMode )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_StopAfterCurrent )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, get_Volume )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_AlwaysOnTop )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_CursorFollowPlayback )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_PlaybackFollowCursor )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_PlaybackTime )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_ReplaygainMode )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_StopAfterCurrent )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbUtils, put_Volume )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "AlwaysOnTop", get_AlwaysOnTop, put_AlwaysOnTop, DefaultPropsFlags() ),
    JS_PSG( "ComponentPath", get_ComponentPath, DefaultPropsFlags() ),
    JS_PSGS( "CursorFollowPlayback", get_CursorFollowPlayback, put_CursorFollowPlayback, DefaultPropsFlags() ),
    JS_PSG( "FoobarPath", get_FoobarPath, DefaultPropsFlags() ),
    JS_PSG( "IsPaused", get_IsPaused, DefaultPropsFlags() ),
    JS_PSG( "IsPlaying", get_IsPlaying, DefaultPropsFlags() ),
    JS_PSGS( "PlaybackFollowCursor", get_PlaybackFollowCursor, put_PlaybackFollowCursor, DefaultPropsFlags() ),
    JS_PSG( "PlaybackLength", get_PlaybackLength, DefaultPropsFlags() ),
    JS_PSGS( "PlaybackTime", get_PlaybackTime, put_PlaybackTime, DefaultPropsFlags() ),
    JS_PSG( "ProfilePath", get_ProfilePath, DefaultPropsFlags() ),
    JS_PSGS( "ReplaygainMode", get_ReplaygainMode, put_ReplaygainMode, DefaultPropsFlags() ),
    JS_PSGS( "StopAfterCurrent", get_StopAfterCurrent, put_StopAfterCurrent, DefaultPropsFlags() ),
    JS_PSGS( "Volume", get_Volume, put_Volume, DefaultPropsFlags() ),
    JS_PS_END
};

}

namespace mozjs
{

JsFbUtils::JsFbUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsFbUtils::~JsFbUtils()
{
}

JSObject* JsFbUtils::Create( JSContext* cx )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsFbUtils( cx ) );

    return jsObj;
}

const JSClass& JsFbUtils::GetClass()
{
    return jsClass;
}

std::optional<JSObject*>
JsFbUtils::AcquireUiSelectionHolder()
{
    ui_selection_holder::ptr holder = ui_selection_manager::get()->acquire();
    JS::RootedObject jsObject( pJsCtx_, JsFbUiSelectionHolder::Create( pJsCtx_, holder ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::nullptr_t>
JsFbUtils::AddDirectory()
{
    standard_commands::main_add_directory();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::AddFiles()
{
    standard_commands::main_add_files();
    return nullptr;
}

std::optional<bool>
JsFbUtils::CheckClipboardContents()
{
    pfc::com_ptr_t<IDataObject> pDO;
    HRESULT hr = OleGetClipboard( pDO.receive_ptr() );
    if ( !SUCCEEDED( hr ) )
    {
        return false;
    }

    bool native;
    DWORD drop_effect = DROPEFFECT_COPY;
    hr = ole_interaction::get()->check_dataobject( pDO, drop_effect, native );
    return SUCCEEDED( hr );
}

std::optional<std::nullptr_t>
JsFbUtils::ClearPlaylist()
{
    standard_commands::main_clear_playlist();
    return nullptr;
}

std::optional<bool>
JsFbUtils::CopyHandleListToClipboard( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject( handles->GetHandleList() );
    if ( !SUCCEEDED( OleSetClipboard( pDO.get_ptr() ) ) )
    {
        return false;
    }

    return true;
}

std::optional<JSObject*>
JsFbUtils::CreateContextMenuManager()
{
    JS::RootedObject jsObject( pJsCtx_, JsContextMenuManager::Create( pJsCtx_ ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::CreateHandleList()
{
    metadb_handle_list items;
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::CreateMainMenuManager()
{
    JS::RootedObject jsObject( pJsCtx_, JsMainMenuManager::Create( pJsCtx_ ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::CreateProfiler( const std::string& name )
{
    JS::RootedObject jsObject( pJsCtx_, JsFbProfiler::Create( pJsCtx_, name ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<uint32_t>
JsFbUtils::DoDragDrop( JsFbMetadbHandleList* handles, uint32_t okEffects )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_cref handles_ptr = handles->GetHandleList();
    if ( !handles_ptr.get_count() || okEffects == DROPEFFECT_NONE )
    {
        return DROPEFFECT_NONE;
    }

    pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject( handles_ptr );
    pfc::com_ptr_t<IDropSourceImpl> pIDropSource = new IDropSourceImpl();

    DWORD returnEffect;
    HRESULT hr = SHDoDragDrop( NULL, pDO.get_ptr(), pIDropSource.get_ptr(), okEffects, &returnEffect );
    return ( DRAGDROP_S_CANCEL == hr ? DROPEFFECT_NONE : returnEffect );
}

std::optional<std::nullptr_t>
JsFbUtils::Exit()
{
    standard_commands::main_exit();
    return nullptr;
}

std::optional<JSObject*>
JsFbUtils::GetClipboardContents( uint32_t hWindow )
{
    auto api = ole_interaction::get();
    pfc::com_ptr_t<IDataObject> pDO;
    metadb_handle_list items;

    HRESULT hr = OleGetClipboard( pDO.receive_ptr() );
    if ( SUCCEEDED( hr ) )
    {
        DWORD drop_effect = DROPEFFECT_COPY;
        bool native;
        hr = api->check_dataobject( pDO, drop_effect, native );
        if ( SUCCEEDED( hr ) )
        {
            dropped_files_data_impl data;
            hr = api->parse_dataobject( pDO, data );
            if ( SUCCEEDED( hr ) )
            {// Such cast will work only on x86
                data.to_handles( items, native, (HWND)hWindow );
            }
        }
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::string>
JsFbUtils::GetDSPPresets()
{
    if ( !helpers::is14() )
    {
        JS_ReportErrorASCII( pJsCtx_, "foobar2000 v1.4+ is required for this method" );
        return std::nullopt;
    }

    json j = json::array();
    auto api = dsp_config_manager_v2::get();
    t_size count = api->get_preset_count();
    pfc::string8 name;

    for ( t_size i = 0; i < count; ++i )
    {
        api->get_preset_name( i, name );

        j.push_back(
            {
                { "active", api->get_selected_preset() == i },
                { "name",  name.get_ptr() }
            } 
        );
    }

    return j.dump();
}

std::optional<JSObject*>
JsFbUtils::GetFocusItem( bool force )
{
    metadb_handle_ptr metadb;
    auto api = playlist_manager::get();

    if ( !api->activeplaylist_get_focus_item_handle( metadb ) && force )
    {
        api->activeplaylist_get_item_handle( metadb, 0 );
    }

    if ( metadb.is_empty() )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, metadb ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::GetLibraryItems()
{
    metadb_handle_list items;
    library_manager::get()->get_all_items( items );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::string>
JsFbUtils::GetLibraryRelativePath( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    pfc::string8_fast temp;
    if ( !library_manager::get()->get_relative_path( ptr, temp ) )
    {
        temp = "";
    }

    return std::string( temp.c_str(), temp.length() );
}

std::optional<JSObject*>
JsFbUtils::GetNowPlaying()
{
    metadb_handle_ptr metadb;
    if ( !playback_control::get()->get_now_playing( metadb ) )
    {
        return nullptr;        
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, metadb ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::string>
JsFbUtils::GetOutputDevices()
{
    if ( !helpers::is14() )
    {
        JS_ReportErrorASCII( pJsCtx_, "foobar2000 v1.4+ is required for this method" );
        return std::nullopt;
    }

    json j;
    auto api = output_manager_v2::get();
    outputCoreConfig_t config;
    api->getCoreConfig( config );

    api->listDevices( [&j, &config]( pfc::string8&& name, auto&& output_id, auto&& device_id )
                      {
                          std::string name_string(name.get_ptr(), name.length());
                          std::string output_string = pfc::print_guid( output_id ).get_ptr();
                          std::string device_string = pfc::print_guid( device_id ).get_ptr();

                          j.push_back(
                              {
                                  { "name", name_string },
                                  { "output_id", "{" + output_string + "}" },
                                  { "device_id", "{" + device_string + "}" },
                                  { "active", config.m_output == output_id && config.m_device == device_id }
                              }
                          );
                      } );

    return j.dump();
}

std::optional<JSObject*>
JsFbUtils::GetQueryItems( JsFbMetadbHandleList* handles, const std::string& query )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_cref handles_ptr = handles->GetHandleList();
    metadb_handle_list dst_list ( handles_ptr );
    search_filter_v2::ptr filter;

    try
    {
        filter = search_filter_manager_v2::get()->create_ex( query.c_str(), 
                                                             new service_impl_t<completion_notify_dummy>(), 
                                                             search_filter_manager_v2::KFlagSuppressNotify );
    }
    catch ( ... )
    {// Error, but no additional info
        return std::nullopt;
    }

    pfc::array_t<bool> mask;
    mask.set_size( dst_list.get_count() );
    filter->test_multi( dst_list, mask.get_ptr() );
    dst_list.filter_mask( mask.get_ptr() );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, dst_list ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::GetSelection()
{
    metadb_handle_list items;
    ui_selection_manager::get()->get_selection( items );

    if ( !items.get_count() )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::Create( pJsCtx_, items[0] ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<JSObject*>
JsFbUtils::GetSelections( uint32_t flags )
{
    metadb_handle_list items;
    ui_selection_manager_v2::get()->get_selection( items, flags );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::Create( pJsCtx_, items ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<uint32_t>
JsFbUtils::GetSelectionType()
{
    const GUID* guids[] = {
        &contextmenu_item::caller_undefined,
        &contextmenu_item::caller_active_playlist_selection,
        &contextmenu_item::caller_active_playlist,
        &contextmenu_item::caller_playlist_manager,
        &contextmenu_item::caller_now_playing,
        &contextmenu_item::caller_keyboard_shortcut_list,
        &contextmenu_item::caller_media_library_viewer,
    };

    GUID type = ui_selection_manager_v2::get()->get_selection_type( 0 );
    for ( t_size i = 0; i < _countof( guids ); ++i )
    {
        if ( *guids[i] == type )
        {
            return i;
        }
    }

    return 0;
}

std::optional<bool>
JsFbUtils::IsLibraryEnabled()
{
    return library_manager::get()->is_library_enabled();
}

std::optional<bool>
JsFbUtils::IsMainMenuCommandChecked( const std::string& command )
{// TODO: inspect get_mainmenu_command_status_by_name_SEH
    t_uint32 status;
    if ( !helpers::get_mainmenu_command_status_by_name_SEH( command.c_str(), status ) )
    {// Error, but no additional info
        return std::nullopt;
    }

    return mainmenu_commands::flag_checked == status 
        || mainmenu_commands::flag_radiochecked == status;
}

std::optional<bool>
JsFbUtils::IsMetadbInMediaLibrary( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    return library_manager::get()->is_item_in_library( handle->GetHandle() );
}

std::optional<std::nullptr_t>
JsFbUtils::LoadPlaylist()
{
    standard_commands::main_load_playlist();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Next()
{
    standard_commands::main_next();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Pause()
{
    standard_commands::main_pause();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Play()
{
    standard_commands::main_play();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::PlayOrPause()
{
    standard_commands::main_play_or_pause();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Prev()
{
    standard_commands::main_previous();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Random()
{
    standard_commands::main_random();
    return nullptr;
}

std::optional<bool>
JsFbUtils::RunContextCommand( const std::string& command, uint32_t flags )
{
    metadb_handle_list dummy_list;
    return helpers::execute_context_command_by_name_SEH( command.c_str(), dummy_list, flags );  
}

std::optional<bool>
JsFbUtils::RunContextCommandWithMetadb( const std::string& command, JS::HandleValue handle, uint32_t flags )
{
    if ( !handle.isObject() )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is invalid" );
        return std::nullopt;
    }

    JS::RootedObject jsObject( pJsCtx_, &handle.toObject() );

    JsFbMetadbHandle* jsHandle = 
        static_cast<JsFbMetadbHandle*>( JS_GetInstancePrivate( pJsCtx_, jsObject, &JsFbMetadbHandle::GetClass(), nullptr ) );
    JsFbMetadbHandleList* jsHandleList =
        static_cast<JsFbMetadbHandleList*>( JS_GetInstancePrivate( pJsCtx_, jsObject, &JsFbMetadbHandleList::GetClass(), nullptr ) );

    if ( !jsHandle || !jsHandleList )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is invalid" );
        return std::nullopt;
    }

    metadb_handle_list handle_list;
    if ( jsHandleList )
    {
        handle_list = jsHandleList->GetHandleList();
    }
    else
    {
        handle_list.add_item( jsHandle->GetHandle() );
    }

    return helpers::execute_context_command_by_name_SEH( command.c_str(), handle_list, flags );
}

std::optional<bool>
JsFbUtils::RunMainMenuCommand( const std::string& command )
{
    return helpers::execute_mainmenu_command_by_name_SEH( command.c_str() );
}

std::optional<std::nullptr_t>
JsFbUtils::SaveIndex()
{
    try
    {
        stats::theAPI()->save_index_data( g_guid_jsp_metadb_index );
    }
    catch ( ... )
    {
        FB2K_console_formatter() << JSP_NAME " v" JSP_VERSION ": Save index fail.";
    }
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::SavePlaylist()
{
    standard_commands::main_save_playlist();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::SetDSPPreset( uint32_t idx )
{
    if ( !helpers::is14() )
    {
        JS_ReportErrorASCII( pJsCtx_, "foobar2000 v1.4+ is required for this method" );
        return std::nullopt;
    }

    auto api = dsp_config_manager_v2::get();
    t_size count = api->get_preset_count();

    if ( idx >= count )
    {
        JS_ReportErrorASCII( pJsCtx_, "Index is out of bounds" );
        return std::nullopt;
    }

    api->select_preset( idx );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::SetOutputDevice( const std::wstring& output, const std::wstring& device )
{
    if ( !helpers::is14() )
    {
        JS_ReportErrorASCII( pJsCtx_, "foobar2000 v1.4+ is required for this method" );
        return std::nullopt;
    }

    GUID output_id, device_id;

    if ( CLSIDFromString( output.c_str(), &output_id ) == NOERROR
         && CLSIDFromString( device.c_str(), &device_id ) == NOERROR )
    {
        output_manager_v2::get()->setCoreConfigDevice( output_id, device_id );
    }
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::ShowConsole()
{
    const GUID guid_main_show_console = { 0x5b652d25, 0xce44, 0x4737,{ 0x99, 0xbb, 0xa3, 0xcf, 0x2a, 0xeb, 0x35, 0xcc } };
    standard_commands::run_main( guid_main_show_console );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::ShowLibrarySearchUI( const std::string& query )
{
    library_search_ui::get()->show( query.c_str() );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::ShowPopupMessage( const std::string& msg, const std::string& title )
{
    popup_msg::g_show( msg.c_str(), title.c_str() );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::ShowPreferences()
{
    standard_commands::main_preferences();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::Stop()
{
    standard_commands::main_stop();
    return nullptr;
}

std::optional<JSObject*>
JsFbUtils::TitleFormat( const std::string& expression )
{
    JS::RootedObject jsObject( pJsCtx_, JsFbTitleFormat::Create( pJsCtx_, expression ) );
    if ( !jsObject )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: failed to create JS object" );
        return std::nullopt;
    }

    return jsObject;
}

std::optional<std::nullptr_t>
JsFbUtils::VolumeDown()
{
    standard_commands::main_volume_down();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::VolumeMute()
{
    standard_commands::main_volume_mute();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::VolumeUp()
{
    standard_commands::main_volume_up();
    return nullptr;
}

std::optional<bool>
JsFbUtils::get_AlwaysOnTop()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_ui_always_on_top, false );
}

std::optional<std::string>
JsFbUtils::get_ComponentPath()
{
    pfc::string8_fast tmp( helpers::get_fb2k_component_path() );
    return std::string( tmp.c_str(), tmp.length() );
}

std::optional<bool>
JsFbUtils::get_CursorFollowPlayback()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_cursor_follows_playback, false );
}

std::optional<std::string>
JsFbUtils::get_FoobarPath()
{
    pfc::string8_fast tmp( helpers::get_fb2k_path() );
    return std::string( tmp.c_str(), tmp.length() );
}

std::optional<bool>
JsFbUtils::get_IsPaused()
{
    return playback_control::get()->is_paused();
}

std::optional<bool>
JsFbUtils::get_IsPlaying()
{
    return playback_control::get()->is_playing();
}

std::optional<bool>
JsFbUtils::get_PlaybackFollowCursor()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_playback_follows_cursor, false );
}

std::optional<double>
JsFbUtils::get_PlaybackLength()
{
    return playback_control::get()->playback_get_length();
}

std::optional<double>
JsFbUtils::get_PlaybackTime()
{
    return playback_control::get()->playback_get_position();
}

std::optional<std::string>
JsFbUtils::get_ProfilePath()
{
    pfc::string8_fast tmp( helpers::get_profile_path() );
    return std::string( tmp.c_str(), tmp.length() );
}

std::optional<uint32_t>
JsFbUtils::get_ReplaygainMode()
{
    t_replaygain_config rg;
    replaygain_manager::get()->get_core_settings( rg );
    return rg.m_source_mode;
}

std::optional<bool>
JsFbUtils::get_StopAfterCurrent()
{
    return playback_control::get()->get_stop_after_current();
}

std::optional<float>
JsFbUtils::get_Volume()
{
    return playback_control::get()->get_volume();
}

std::optional<std::nullptr_t>
JsFbUtils::put_AlwaysOnTop( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_ui_always_on_top, p );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_CursorFollowPlayback( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_cursor_follows_playback, p );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_PlaybackFollowCursor( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_playback_follows_cursor, p );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_PlaybackTime( double time )
{
    playback_control::get()->playback_seek( time );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_ReplaygainMode( uint32_t p )
{
    switch ( p )
    {
    case 0:
        standard_commands::main_rg_disable();
        break;
    case 1:
        standard_commands::main_rg_set_track();
        break;
    case 2:
        standard_commands::main_rg_set_album();
        break;
    case 3:
        standard_commands::run_main( standard_commands::guid_main_rg_byorder );
        break;
    default:
    {
        JS_ReportErrorASCII( pJsCtx_, "Invalid replay gain mode: %d", p );
        return std::nullopt;
    }
    }

    playback_control_v3::get()->restart();
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_StopAfterCurrent( bool p )
{
    playback_control::get()->set_stop_after_current( p );
    return nullptr;
}

std::optional<std::nullptr_t>
JsFbUtils::put_Volume( float value )
{
    playback_control::get()->set_volume( value );
    return nullptr;
}

}
