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
#include <utils/string_helpers.h>

#include <com_objects/drop_source_impl.h>

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
    JsFbUtils::FinalizeJsObject,
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

MJS_DEFINE_JS_FN_FROM_NATIVE( AcquireUiSelectionHolder, JsFbUtils::AcquireUiSelectionHolder )
MJS_DEFINE_JS_FN_FROM_NATIVE( AddDirectory, JsFbUtils::AddDirectory )
MJS_DEFINE_JS_FN_FROM_NATIVE( AddFiles, JsFbUtils::AddFiles )
MJS_DEFINE_JS_FN_FROM_NATIVE( CheckClipboardContents, JsFbUtils::CheckClipboardContents )
MJS_DEFINE_JS_FN_FROM_NATIVE( ClearPlaylist, JsFbUtils::ClearPlaylist )
MJS_DEFINE_JS_FN_FROM_NATIVE( CopyHandleListToClipboard, JsFbUtils::CopyHandleListToClipboard )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateContextMenuManager, JsFbUtils::CreateContextMenuManager )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateHandleList, JsFbUtils::CreateHandleList )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateMainMenuManager, JsFbUtils::CreateMainMenuManager )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CreateProfiler, JsFbUtils::CreateProfiler, JsFbUtils::CreateProfilerWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( DoDragDrop, JsFbUtils::DoDragDrop )
MJS_DEFINE_JS_FN_FROM_NATIVE( Exit, JsFbUtils::Exit )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetClipboardContents, JsFbUtils::GetClipboardContents )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDSPPresets, JsFbUtils::GetDSPPresets )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetFocusItem, JsFbUtils::GetFocusItem, JsFbUtils::GetFocusItemWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetLibraryItems, JsFbUtils::GetLibraryItems )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetLibraryRelativePath, JsFbUtils::GetLibraryRelativePath )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetNowPlaying, JsFbUtils::GetNowPlaying )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetOutputDevices, JsFbUtils::GetOutputDevices )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetQueryItems, JsFbUtils::GetQueryItems )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSelection, JsFbUtils::GetSelection )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetSelections, JsFbUtils::GetSelections, JsFbUtils::GetSelectionsWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetSelectionType, JsFbUtils::GetSelectionType )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsLibraryEnabled, JsFbUtils::IsLibraryEnabled )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsMainMenuCommandChecked, JsFbUtils::IsMainMenuCommandChecked )
MJS_DEFINE_JS_FN_FROM_NATIVE( IsMetadbInMediaLibrary, JsFbUtils::IsMetadbInMediaLibrary )
MJS_DEFINE_JS_FN_FROM_NATIVE( LoadPlaylist, JsFbUtils::LoadPlaylist )
MJS_DEFINE_JS_FN_FROM_NATIVE( Next, JsFbUtils::Next )
MJS_DEFINE_JS_FN_FROM_NATIVE( Pause, JsFbUtils::Pause )
MJS_DEFINE_JS_FN_FROM_NATIVE( Play, JsFbUtils::Play )
MJS_DEFINE_JS_FN_FROM_NATIVE( PlayOrPause, JsFbUtils::PlayOrPause )
MJS_DEFINE_JS_FN_FROM_NATIVE( Prev, JsFbUtils::Prev )
MJS_DEFINE_JS_FN_FROM_NATIVE( Random, JsFbUtils::Random )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RunContextCommand, JsFbUtils::RunContextCommand, JsFbUtils::RunContextCommandWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RunContextCommandWithMetadb, JsFbUtils::RunContextCommandWithMetadb, JsFbUtils::RunContextCommandWithMetadbWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( RunMainMenuCommand, JsFbUtils::RunMainMenuCommand )
MJS_DEFINE_JS_FN_FROM_NATIVE( SavePlaylist, JsFbUtils::SavePlaylist )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetDSPPreset, JsFbUtils::SetDSPPreset )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetOutputDevice, JsFbUtils::SetOutputDevice )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowConsole, JsFbUtils::ShowConsole )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowLibrarySearchUI, JsFbUtils::ShowLibrarySearchUI )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( ShowPopupMessage, JsFbUtils::ShowPopupMessage, JsFbUtils::ShowPopupMessageWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowPreferences, JsFbUtils::ShowPreferences )
MJS_DEFINE_JS_FN_FROM_NATIVE( Stop, JsFbUtils::Stop )
MJS_DEFINE_JS_FN_FROM_NATIVE( TitleFormat, JsFbUtils::TitleFormat )
MJS_DEFINE_JS_FN_FROM_NATIVE( VolumeDown, JsFbUtils::VolumeDown )
MJS_DEFINE_JS_FN_FROM_NATIVE( VolumeMute, JsFbUtils::VolumeMute )
MJS_DEFINE_JS_FN_FROM_NATIVE( VolumeUp, JsFbUtils::VolumeUp )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "AcquireUiSelectionHolder", AcquireUiSelectionHolder, 0, DefaultPropsFlags() ),
    JS_FN( "AddDirectory", AddDirectory, 0, DefaultPropsFlags() ),
    JS_FN( "AddFiles", AddFiles, 0, DefaultPropsFlags() ),
    JS_FN( "CheckClipboardContents", CheckClipboardContents, 0, DefaultPropsFlags() ),
    JS_FN( "ClearPlaylist", ClearPlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "CopyHandleListToClipboard", CopyHandleListToClipboard, 1, DefaultPropsFlags() ),
    JS_FN( "CreateContextMenuManager", CreateContextMenuManager, 0, DefaultPropsFlags() ),
    JS_FN( "CreateHandleList", CreateHandleList, 0, DefaultPropsFlags() ),
    JS_FN( "CreateMainMenuManager", CreateMainMenuManager, 0, DefaultPropsFlags() ),
    JS_FN( "CreateProfiler", CreateProfiler, 0, DefaultPropsFlags() ),
    JS_FN( "DoDragDrop", DoDragDrop, 2, DefaultPropsFlags() ),
    JS_FN( "Exit", Exit, 0, DefaultPropsFlags() ),
    JS_FN( "GetClipboardContents", GetClipboardContents, 0, DefaultPropsFlags() ),
    JS_FN( "GetDSPPresets", GetDSPPresets, 0, DefaultPropsFlags() ),
    JS_FN( "GetFocusItem", GetFocusItem, 0, DefaultPropsFlags() ),
    JS_FN( "GetLibraryItems", GetLibraryItems, 0, DefaultPropsFlags() ),
    JS_FN( "GetLibraryRelativePath", GetLibraryRelativePath, 1, DefaultPropsFlags() ),
    JS_FN( "GetNowPlaying", GetNowPlaying, 0, DefaultPropsFlags() ),
    JS_FN( "GetOutputDevices", GetOutputDevices, 0, DefaultPropsFlags() ),
    JS_FN( "GetQueryItems", GetQueryItems, 2, DefaultPropsFlags() ),
    JS_FN( "GetSelection", GetSelection, 0, DefaultPropsFlags() ),
    JS_FN( "GetSelections", GetSelections, 0, DefaultPropsFlags() ),
    JS_FN( "GetSelectionType", GetSelectionType, 0, DefaultPropsFlags() ),
    JS_FN( "IsLibraryEnabled", IsLibraryEnabled, 0, DefaultPropsFlags() ),
    JS_FN( "IsMainMenuCommandChecked", IsMainMenuCommandChecked, 1, DefaultPropsFlags() ),
    JS_FN( "IsMetadbInMediaLibrary", IsMetadbInMediaLibrary, 1, DefaultPropsFlags() ),
    JS_FN( "LoadPlaylist", LoadPlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "Next", Next, 0, DefaultPropsFlags() ),
    JS_FN( "Pause", Pause, 0, DefaultPropsFlags() ),
    JS_FN( "Play", Play, 0, DefaultPropsFlags() ),
    JS_FN( "PlayOrPause", PlayOrPause, 0, DefaultPropsFlags() ),
    JS_FN( "Prev", Prev, 0, DefaultPropsFlags() ),
    JS_FN( "Random", Random, 0, DefaultPropsFlags() ),
    JS_FN( "RunContextCommand", RunContextCommand, 1, DefaultPropsFlags() ),
    JS_FN( "RunContextCommandWithMetadb", RunContextCommandWithMetadb, 2, DefaultPropsFlags() ),
    JS_FN( "RunMainMenuCommand", RunMainMenuCommand, 1, DefaultPropsFlags() ),
    JS_FN( "SavePlaylist", SavePlaylist, 0, DefaultPropsFlags() ),
    JS_FN( "SetDSPPreset", SetDSPPreset, 1, DefaultPropsFlags() ),
    JS_FN( "SetOutputDevice", SetOutputDevice, 2, DefaultPropsFlags() ),
    JS_FN( "ShowConsole", ShowConsole, 0, DefaultPropsFlags() ),
    JS_FN( "ShowLibrarySearchUI", ShowLibrarySearchUI, 1, DefaultPropsFlags() ),
    JS_FN( "ShowPopupMessage", ShowPopupMessage, 1, DefaultPropsFlags() ),
    JS_FN( "ShowPreferences", ShowPreferences, 0, DefaultPropsFlags() ),
    JS_FN( "Stop", Stop, 0, DefaultPropsFlags() ),
    JS_FN( "TitleFormat", TitleFormat, 1, DefaultPropsFlags() ),
    JS_FN( "VolumeDown", VolumeDown, 0, DefaultPropsFlags() ),
    JS_FN( "VolumeMute", VolumeMute, 0, DefaultPropsFlags() ),
    JS_FN( "VolumeUp", VolumeUp, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_AlwaysOnTop, JsFbUtils::get_AlwaysOnTop )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_ComponentPath, JsFbUtils::get_ComponentPath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_CursorFollowPlayback, JsFbUtils::get_CursorFollowPlayback )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_FoobarPath, JsFbUtils::get_FoobarPath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsPaused, JsFbUtils::get_IsPaused )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsPlaying, JsFbUtils::get_IsPlaying )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaybackFollowCursor, JsFbUtils::get_PlaybackFollowCursor )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaybackLength, JsFbUtils::get_PlaybackLength )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PlaybackTime, JsFbUtils::get_PlaybackTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_ProfilePath, JsFbUtils::get_ProfilePath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_ReplaygainMode, JsFbUtils::get_ReplaygainMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_StopAfterCurrent, JsFbUtils::get_StopAfterCurrent )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Volume, JsFbUtils::get_Volume )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_AlwaysOnTop, JsFbUtils::put_AlwaysOnTop )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_CursorFollowPlayback, JsFbUtils::put_CursorFollowPlayback )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_PlaybackFollowCursor, JsFbUtils::put_PlaybackFollowCursor )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_PlaybackTime, JsFbUtils::put_PlaybackTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_ReplaygainMode, JsFbUtils::put_ReplaygainMode )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_StopAfterCurrent, JsFbUtils::put_StopAfterCurrent )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Volume, JsFbUtils::put_Volume )

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

} // namespace

namespace mozjs
{

const JSClass JsFbUtils::JsClass = jsClass;
const JSFunctionSpec* JsFbUtils::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbUtils::JsProperties = jsProperties;

JsFbUtils::JsFbUtils( JSContext* cx )
    : pJsCtx_( cx )
{
}

JsFbUtils::~JsFbUtils()
{
}

std::unique_ptr<JsFbUtils>
JsFbUtils::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsFbUtils>( new JsFbUtils( cx ) );
}

size_t JsFbUtils::GetInternalSize()
{
    return 0;
}

JSObject* JsFbUtils::AcquireUiSelectionHolder()
{
    ui_selection_holder::ptr holder = ui_selection_manager::get()->acquire();
    JS::RootedObject jsObject( pJsCtx_, JsFbUiSelectionHolder::CreateJs( pJsCtx_, holder ) );
    assert( jsObject );

    return jsObject;
}

void JsFbUtils::AddDirectory()
{
    standard_commands::main_add_directory();
}

void JsFbUtils::AddFiles()
{
    standard_commands::main_add_files();
}

bool JsFbUtils::CheckClipboardContents()
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

void JsFbUtils::ClearPlaylist()
{
    standard_commands::main_clear_playlist();
}

bool JsFbUtils::CopyHandleListToClipboard( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject( handles->GetHandleList() );
    return SUCCEEDED( OleSetClipboard( pDO.get_ptr() ) );
}

JSObject* JsFbUtils::CreateContextMenuManager()
{
    JS::RootedObject jsObject( pJsCtx_, JsContextMenuManager::CreateJs( pJsCtx_ ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::CreateHandleList()
{
    metadb_handle_list items;
    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, items ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::CreateMainMenuManager()
{
    JS::RootedObject jsObject( pJsCtx_, JsMainMenuManager::CreateJs( pJsCtx_ ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::CreateProfiler( const pfc::string8_fast& name )
{
    JS::RootedObject jsObject( pJsCtx_, JsFbProfiler::CreateJs( pJsCtx_, name ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::CreateProfilerWithOpt( size_t optArgCount, const pfc::string8_fast& name )
{
    switch ( optArgCount )
    {
    case 0:
        return CreateProfiler( name );
    case 1:
        return CreateProfiler();
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

uint32_t JsFbUtils::DoDragDrop( JsFbMetadbHandleList* handles, uint32_t okEffects )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadb_handle_list_cref handles_ptr = handles->GetHandleList();
    if ( !handles_ptr.get_count() || okEffects == DROPEFFECT_NONE )
    {
        return DROPEFFECT_NONE;
    }

    pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject( handles_ptr );
    pfc::com_ptr_t<IDropSourceImpl> pIDropSource = new IDropSourceImpl();

    DWORD returnEffect;
    HRESULT hr = SHDoDragDrop( nullptr, pDO.get_ptr(), pIDropSource.get_ptr(), okEffects, &returnEffect );
    return ( DRAGDROP_S_CANCEL == hr ? DROPEFFECT_NONE : returnEffect );
}

void JsFbUtils::Exit()
{
    standard_commands::main_exit();
}

JSObject* JsFbUtils::GetClipboardContents( uint32_t hWindow )
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
            { // Such cast will work only on x86
                data.to_handles( items, native, (HWND)hWindow );
            }
        }
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, items ) );
    if ( !jsObject )
    {
        throw smp::SmpException( "Internal error: failed to create JS object" );
    }

    return jsObject;
}

pfc::string8_fast JsFbUtils::GetDSPPresets()
{
    using json = nlohmann::json;

    if ( !static_api_test_t<output_manager_v2>() )
    {
        throw smp::SmpException( "This method requires foobar2000 v1.4 or later" );
    }

    json j = json::array();
    auto api = output_manager_v2::get();
    outputCoreConfig_t config;
    api->getCoreConfig( config );
    api->listDevices( [&j, &config]( const char* fullName, const GUID& output_id, const GUID& device_id ) {
        pfc::string8 output_string, device_string;
        output_string << "{" << pfc::print_guid( output_id ) << "}";
        device_string << "{" << pfc::print_guid( device_id ) << "}";
        j.push_back( { { "name", fullName },
                       { "output_id", output_string.get_ptr() },
                       { "device_id", device_string.get_ptr() },
                       { "active", config.m_output == output_id && config.m_device == device_id } } );
    } );

    return j.dump().c_str();
}

JSObject* JsFbUtils::GetFocusItem( bool force )
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

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::CreateJs( pJsCtx_, metadb ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::GetFocusItemWithOpt( size_t optArgCount, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return GetFocusItem( force );
    case 1:
        return GetFocusItem();
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

JSObject* JsFbUtils::GetLibraryItems()
{
    metadb_handle_list items;
    library_manager::get()->get_all_items( items );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, items ) );
    assert( jsObject );

    return jsObject;
}

pfc::string8_fast JsFbUtils::GetLibraryRelativePath( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    metadb_handle_ptr ptr = handle->GetHandle();
    pfc::string8_fast temp;
    if ( !library_manager::get()->get_relative_path( ptr, temp ) )
    {
        temp = "";
    }

    return pfc::string8_fast( temp.c_str(), temp.length() );
}

JSObject* JsFbUtils::GetNowPlaying()
{
    metadb_handle_ptr metadb;
    if ( !playback_control::get()->get_now_playing( metadb ) )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::CreateJs( pJsCtx_, metadb ) );
    assert( jsObject );

    return jsObject;
}

pfc::string8_fast JsFbUtils::GetOutputDevices()
{
    using json = nlohmann::json;

    if ( !static_api_test_t<output_manager_v2>() )
    {
        throw smp::SmpException( "This method requires foobar2000 v1.4 or later" );
    }

    json j = json::array();
    auto api = output_manager_v2::get();
    outputCoreConfig_t config;
    api->getCoreConfig( config );

    api->listDevices( [&j, &config]( pfc::string8&& name, auto&& output_id, auto&& device_id ) {
        std::string name_string( name.get_ptr(), name.length() );
        std::string output_string = pfc::print_guid( output_id ).get_ptr();
        std::string device_string = pfc::print_guid( device_id ).get_ptr();

        j.push_back(
            { { "name", name_string },
              { "output_id", "{" + output_string + "}" },
              { "device_id", "{" + device_string + "}" },
              { "active", config.m_output == output_id && config.m_device == device_id } } );
    } );

    return j.dump().c_str();
}

JSObject* JsFbUtils::GetQueryItems( JsFbMetadbHandleList* handles, const pfc::string8_fast& query )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadb_handle_list_cref handles_ptr = handles->GetHandleList();
    metadb_handle_list dst_list( handles_ptr );
    search_filter_v2::ptr filter;

    try
    {
        filter = search_filter_manager_v2::get()->create_ex( query.c_str(),
                                                             new service_impl_t<completion_notify_dummy>(),
                                                             search_filter_manager_v2::KFlagSuppressNotify );
    }
    catch ( ... )
    { // TODO: Error, but no additional info
        throw smp::SmpException( "" );
    }

    pfc::array_t<bool> mask;
    mask.set_size( dst_list.get_count() );
    filter->test_multi( dst_list, mask.get_ptr() );
    dst_list.filter_mask( mask.get_ptr() );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, dst_list ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::GetSelection()
{
    metadb_handle_list items;
    ui_selection_manager::get()->get_selection( items );

    if ( !items.get_count() )
    {
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandle::CreateJs( pJsCtx_, items[0] ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::GetSelections( uint32_t flags )
{
    metadb_handle_list items;
    ui_selection_manager_v2::get()->get_selection( items, flags );

    JS::RootedObject jsObject( pJsCtx_, JsFbMetadbHandleList::CreateJs( pJsCtx_, items ) );
    assert( jsObject );

    return jsObject;
}

JSObject* JsFbUtils::GetSelectionsWithOpt( size_t optArgCount, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return GetSelections( flags );
    case 1:
        return GetSelections();
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

uint32_t JsFbUtils::GetSelectionType()
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

bool JsFbUtils::IsLibraryEnabled()
{
    return library_manager::get()->is_library_enabled();
}

bool JsFbUtils::IsMainMenuCommandChecked( const pfc::string8_fast& command )
{ // TODO: inspect get_mainmenu_command_status_by_name_SEH
    t_uint32 status;
    if ( !helpers::get_mainmenu_command_status_by_name_SEH( command.c_str(), status ) )
    { // TODO: Error, but no additional info
        throw smp::SmpException( "" );
    }

    return mainmenu_commands::flag_checked == status
           || mainmenu_commands::flag_radiochecked == status;
}

bool JsFbUtils::IsMetadbInMediaLibrary( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    return library_manager::get()->is_item_in_library( handle->GetHandle() );
}

void JsFbUtils::LoadPlaylist()
{
    standard_commands::main_load_playlist();
}

void JsFbUtils::Next()
{
    standard_commands::main_next();
}

void JsFbUtils::Pause()
{
    standard_commands::main_pause();
}

void JsFbUtils::Play()
{
    standard_commands::main_play();
}

void JsFbUtils::PlayOrPause()
{
    standard_commands::main_play_or_pause();
}

void JsFbUtils::Prev()
{
    standard_commands::main_previous();
}

void JsFbUtils::Random()
{
    standard_commands::main_random();
}

bool JsFbUtils::RunContextCommand( const pfc::string8_fast& command, uint32_t flags )
{
    metadb_handle_list dummy_list;
    return helpers::execute_context_command_by_name_SEH( command.c_str(), dummy_list, flags );
}

bool JsFbUtils::RunContextCommandWithOpt( size_t optArgCount, const pfc::string8_fast& command, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return RunContextCommand( command, flags );
    case 1:
        return RunContextCommand( command );
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

bool JsFbUtils::RunContextCommandWithMetadb( const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags )
{
    if ( !handle.isObject() )
    {
        throw smp::SmpException( "handle argument is invalid" );
    }

    JS::RootedObject jsObject( pJsCtx_, &handle.toObject() );

    JsFbMetadbHandle* jsHandle = GetInnerInstancePrivate<JsFbMetadbHandle>( pJsCtx_, jsObject );
    JsFbMetadbHandleList* jsHandleList = GetInnerInstancePrivate<JsFbMetadbHandleList>( pJsCtx_, jsObject );

    if ( !jsHandle && !jsHandleList )
    {
        throw smp::SmpException( "handle argument is invalid" );
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

bool JsFbUtils::RunContextCommandWithMetadbWithOpt( size_t optArgCount, const pfc::string8_fast& command, JS::HandleValue handle, uint32_t flags )
{
    switch ( optArgCount )
    {
    case 0:
        return RunContextCommandWithMetadb( command, handle, flags );
    case 1:
        return RunContextCommandWithMetadb( command, handle );
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

bool JsFbUtils::RunMainMenuCommand( const pfc::string8_fast& command )
{
    return helpers::execute_mainmenu_command_by_name_SEH( command.c_str() );
}

void JsFbUtils::SavePlaylist()
{
    standard_commands::main_save_playlist();
}

void JsFbUtils::SetDSPPreset( uint32_t idx )
{
    if ( !static_api_test_t<dsp_config_manager_v2>() )
    {
        throw smp::SmpException( "This method requires foobar2000 v1.4 or later" );
    }

    auto api = dsp_config_manager_v2::get();
    t_size count = api->get_preset_count();

    if ( idx >= count )
    {
        throw smp::SmpException( "Index is out of bounds" );
    }

    api->select_preset( idx );
}

void JsFbUtils::SetOutputDevice( const std::wstring& output, const std::wstring& device )
{
    if ( !static_api_test_t<output_manager_v2>() )
    {
        throw smp::SmpException( "This method requires foobar2000 v1.4 or later" );
    }

    GUID output_id, device_id;

    if ( CLSIDFromString( output.c_str(), &output_id ) == NOERROR
         && CLSIDFromString( device.c_str(), &device_id ) == NOERROR )
    {
        output_manager_v2::get()->setCoreConfigDevice( output_id, device_id );
    }
}

void JsFbUtils::ShowConsole()
{
    const GUID guid_main_show_console = { 0x5b652d25, 0xce44, 0x4737, { 0x99, 0xbb, 0xa3, 0xcf, 0x2a, 0xeb, 0x35, 0xcc } };
    standard_commands::run_main( guid_main_show_console );
}

void JsFbUtils::ShowLibrarySearchUI( const pfc::string8_fast& query )
{
    library_search_ui::get()->show( query.c_str() );
}

void JsFbUtils::ShowPopupMessage( const pfc::string8_fast& msg, const pfc::string8_fast& title )
{
    popup_msg::g_show( msg.c_str(), title.c_str() );
}

void JsFbUtils::ShowPopupMessageWithOpt( size_t optArgCount, const pfc::string8_fast& msg, const pfc::string8_fast& title )
{
    switch ( optArgCount )
    {
    case 0:
        return ShowPopupMessage( msg, title );
    case 1:
        return ShowPopupMessage( msg );
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsFbUtils::ShowPreferences()
{
    standard_commands::main_preferences();
}

void JsFbUtils::Stop()
{
    standard_commands::main_stop();
}

JSObject* JsFbUtils::TitleFormat( const pfc::string8_fast& expression )
{
    JS::RootedObject jsObject( pJsCtx_, JsFbTitleFormat::CreateJs( pJsCtx_, expression ) );
    assert( jsObject );

    return jsObject;
}

void JsFbUtils::VolumeDown()
{
    standard_commands::main_volume_down();
}

void JsFbUtils::VolumeMute()
{
    standard_commands::main_volume_mute();
}

void JsFbUtils::VolumeUp()
{
    standard_commands::main_volume_up();
}

bool JsFbUtils::get_AlwaysOnTop()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_ui_always_on_top, false );
}

pfc::string8_fast JsFbUtils::get_ComponentPath()
{
    pfc::string8_fast tmp( helpers::get_fb2k_component_path() );
    return pfc::string8_fast( tmp.c_str(), tmp.length() );
}

bool JsFbUtils::get_CursorFollowPlayback()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_cursor_follows_playback, false );
}

pfc::string8_fast JsFbUtils::get_FoobarPath()
{
    pfc::string8_fast tmp( helpers::get_fb2k_path() );
    return pfc::string8_fast( tmp.c_str(), tmp.length() );
}

bool JsFbUtils::get_IsPaused()
{
    return playback_control::get()->is_paused();
}

bool JsFbUtils::get_IsPlaying()
{
    return playback_control::get()->is_playing();
}

bool JsFbUtils::get_PlaybackFollowCursor()
{
    return config_object::g_get_data_bool_simple( standard_config_objects::bool_playback_follows_cursor, false );
}

double JsFbUtils::get_PlaybackLength()
{
    return playback_control::get()->playback_get_length();
}

double JsFbUtils::get_PlaybackTime()
{
    return playback_control::get()->playback_get_position();
}

pfc::string8_fast JsFbUtils::get_ProfilePath()
{
    pfc::string8_fast tmp( helpers::get_profile_path() );
    return pfc::string8_fast( tmp.c_str(), tmp.length() );
}

uint32_t JsFbUtils::get_ReplaygainMode()
{
    t_replaygain_config rg;
    replaygain_manager::get()->get_core_settings( rg );
    return rg.m_source_mode;
}

bool JsFbUtils::get_StopAfterCurrent()
{
    return playback_control::get()->get_stop_after_current();
}

float JsFbUtils::get_Volume()
{
    return playback_control::get()->get_volume();
}

void JsFbUtils::put_AlwaysOnTop( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_ui_always_on_top, p );
}

void JsFbUtils::put_CursorFollowPlayback( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_cursor_follows_playback, p );
}

void JsFbUtils::put_PlaybackFollowCursor( bool p )
{
    config_object::g_set_data_bool( standard_config_objects::bool_playback_follows_cursor, p );
}

void JsFbUtils::put_PlaybackTime( double time )
{
    playback_control::get()->playback_seek( time );
}

void JsFbUtils::put_ReplaygainMode( uint32_t p )
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
        throw smp::SmpException( smp::string::Formatter() << "Invalid replay gain mode: " << p );
    }
    }

    playback_control_v3::get()->restart();
}

void JsFbUtils::put_StopAfterCurrent( bool p )
{
    playback_control::get()->set_stop_after_current( p );
}

void JsFbUtils::put_Volume( float value )
{
    playback_control::get()->set_volume( value );
}

} // namespace mozjs
