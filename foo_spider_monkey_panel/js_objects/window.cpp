#include <stdafx.h>

#include "window.h"

#include <com_objects/host_drop_target.h>
#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_font.h>
#include <js_objects/internal/fb_properties.h>
#include <js_objects/menu_object.h>
#include <js_objects/theme_manager.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <utils/array_x.h>
#include <utils/gdi_helpers.h>
#include <utils/scope_helpers.h>
#include <utils/winapi_error_helpers.h>

#include <host_timer_dispatcher.h>
#include <js_panel_window.h>
#include <message_manager.h>
#include <user_message.h>

using namespace smp;

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
    JsWindow::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    JsWindow::Trace
};

JSClass jsClass = {
    "Window",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( ClearInterval, JsWindow::ClearInterval )
MJS_DEFINE_JS_FN_FROM_NATIVE( ClearTimeout, JsWindow::ClearTimeout )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreatePopupMenu, JsWindow::CreatePopupMenu )
MJS_DEFINE_JS_FN_FROM_NATIVE( CreateThemeManager, JsWindow::CreateThemeManager )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( CreateTooltip, JsWindow::CreateTooltip, JsWindow::CreateTooltipWithOpt, 3 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( DefinePanel, JsWindow::DefinePanel, JsWindow::DefinePanelWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetColourCUI, JsWindow::GetColourCUI, JsWindow::GetColourCUIWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetColourDUI, JsWindow::GetColourDUI )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetFontCUI, JsWindow::GetFontCUI, JsWindow::GetFontCUIWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetFontDUI, JsWindow::GetFontDUI )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( GetProperty, JsWindow::GetProperty, JsWindow::GetPropertyWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( NotifyOthers, JsWindow::NotifyOthers )
MJS_DEFINE_JS_FN_FROM_NATIVE( Reload, JsWindow::Reload )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Repaint, JsWindow::Repaint, JsWindow::RepaintWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( RepaintRect, JsWindow::RepaintRect, JsWindow::RepaintRectWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetCursor, JsWindow::SetCursor )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetInterval, JsWindow::SetInterval, JsWindow::SetIntervalWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetProperty, JsWindow::SetProperty, JsWindow::SetPropertyWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetTimeout, JsWindow::SetTimeout, JsWindow::SetTimeoutWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowConfigure, JsWindow::ShowConfigure )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowProperties, JsWindow::ShowProperties )

constexpr auto jsFunctions = smp::to_array<JSFunctionSpec>(
    {
        JS_FN( "ClearInterval", ClearInterval, 1, kDefaultPropsFlags ),
        JS_FN( "ClearTimeout", ClearTimeout, 1, kDefaultPropsFlags ),
        JS_FN( "CreatePopupMenu", CreatePopupMenu, 0, kDefaultPropsFlags ),
        JS_FN( "CreateThemeManager", CreateThemeManager, 1, kDefaultPropsFlags ),
        JS_FN( "CreateTooltip", CreateTooltip, 0, kDefaultPropsFlags ),
        JS_FN( "DefinePanel", DefinePanel, 1, kDefaultPropsFlags ),
        JS_FN( "GetColourCUI", GetColourCUI, 1, kDefaultPropsFlags ),
        JS_FN( "GetColourDUI", GetColourDUI, 1, kDefaultPropsFlags ),
        JS_FN( "GetFontCUI", GetFontCUI, 1, kDefaultPropsFlags ),
        JS_FN( "GetFontDUI", GetFontDUI, 1, kDefaultPropsFlags ),
        JS_FN( "GetProperty", GetProperty, 1, kDefaultPropsFlags ),
        JS_FN( "NotifyOthers", NotifyOthers, 2, kDefaultPropsFlags ),
        JS_FN( "Reload", Reload, 0, kDefaultPropsFlags ),
        JS_FN( "Repaint", Repaint, 0, kDefaultPropsFlags ),
        JS_FN( "RepaintRect", RepaintRect, 4, kDefaultPropsFlags ),
        JS_FN( "SetCursor", SetCursor, 1, kDefaultPropsFlags ),
        JS_FN( "SetInterval", SetInterval, 2, kDefaultPropsFlags ),
        JS_FN( "SetProperty", SetProperty, 1, kDefaultPropsFlags ),
        JS_FN( "SetTimeout", SetTimeout, 2, kDefaultPropsFlags ),
        JS_FN( "ShowConfigure", ShowConfigure, 0, kDefaultPropsFlags ),
        JS_FN( "ShowProperties", ShowProperties, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_DlgCode, JsWindow::get_DlgCode )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsWindow::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_ID, JsWindow::get_ID )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_InstanceType, JsWindow::get_InstanceType )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsTransparent, JsWindow::get_IsTransparent )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsVisible, JsWindow::get_IsVisible )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MaxHeight, JsWindow::get_MaxHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MaxWidth, JsWindow::get_MaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MemoryLimit, JsWindow::get_MemoryLimit )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MinHeight, JsWindow::get_MinHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MinWidth, JsWindow::get_MinWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsWindow::get_Name )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_PanelMemoryUsage, JsWindow::get_PanelMemoryUsage )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Tooltip, JsWindow::get_Tooltip )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_TotalMemoryUsage, JsWindow::get_TotalMemoryUsage )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsWindow::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_DlgCode, JsWindow::put_DlgCode )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MaxHeight, JsWindow::put_MaxHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MaxWidth, JsWindow::put_MaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MinHeight, JsWindow::put_MinHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MinWidth, JsWindow::put_MinWidth )

constexpr auto jsProperties = smp::to_array<JSPropertySpec>(
    {
        JS_PSGS( "DlgCode", get_DlgCode, put_DlgCode, kDefaultPropsFlags ),
        JS_PSG( "Height", get_Height, kDefaultPropsFlags ),
        JS_PSG( "ID", get_ID, kDefaultPropsFlags ),
        JS_PSG( "InstanceType", get_InstanceType, kDefaultPropsFlags ),
        JS_PSG( "IsTransparent", get_IsTransparent, kDefaultPropsFlags ),
        JS_PSG( "IsVisible", get_IsVisible, kDefaultPropsFlags ),
        JS_PSGS( "MaxHeight", get_MaxHeight, put_MaxHeight, kDefaultPropsFlags ),
        JS_PSGS( "MaxWidth", get_MaxWidth, put_MaxWidth, kDefaultPropsFlags ),
        JS_PSG( "MemoryLimit", get_MemoryLimit, kDefaultPropsFlags ),
        JS_PSGS( "MinHeight", get_MinHeight, put_MinHeight, kDefaultPropsFlags ),
        JS_PSGS( "MinWidth", get_MinWidth, put_MinWidth, kDefaultPropsFlags ),
        JS_PSG( "Name", get_Name, kDefaultPropsFlags ),
        JS_PSG( "PanelMemoryUsage", get_PanelMemoryUsage, kDefaultPropsFlags ),
        JS_PSG( "Tooltip", get_Tooltip, kDefaultPropsFlags ),
        JS_PSG( "TotalMemoryUsage", get_TotalMemoryUsage, kDefaultPropsFlags ),
        JS_PSG( "Width", get_Width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsWindow::JsClass = jsClass;
const JSFunctionSpec* JsWindow::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsWindow::JsProperties = jsProperties.data();

JsWindow::JsWindow( JSContext* cx, smp::panel::js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
    , fbProperties_( std::move( fbProperties ) )
{
}

std::unique_ptr<JsWindow>
JsWindow::CreateNative( JSContext* cx, smp::panel::js_panel_window& parentPanel )
{
    std::unique_ptr<FbProperties> fbProperties = FbProperties::Create( cx, parentPanel );
    if ( !fbProperties )
    { // report in Create
        return nullptr;
    }

    return std::unique_ptr<JsWindow>( new JsWindow( cx, parentPanel, std::move( fbProperties ) ) );
}

size_t JsWindow::GetInternalSize( const smp::panel::js_panel_window& )
{
    return sizeof( FbProperties );
}

void JsWindow::Trace( JSTracer* trc, JSObject* obj )
{
    auto x = static_cast<JsWindow*>( JS_GetPrivate( obj ) );
    if ( x && x->fbProperties_ )
    {
        x->fbProperties_->Trace( trc );
    }
}

void JsWindow::PrepareForGc()
{
    if ( fbProperties_ )
    {
        fbProperties_->PrepareForGc();
        fbProperties_.reset();
    }
    if ( dropTargetHandler_ )
    {
        dropTargetHandler_->RevokeDragDrop();
        dropTargetHandler_.Release();
    }
    if ( fbProperties_ )
    {
        assert( pNativeTooltip_ );
        pNativeTooltip_->PrepareForGc();
        jsTooltip_.reset();
    }

    isFinalized_ = true;
}

void JsWindow::ClearInterval( uint32_t intervalId )
{
    if ( isFinalized_ )
    {
        return;
    }

    HostTimerDispatcher::Get().killTimer( intervalId );
}

void JsWindow::ClearTimeout( uint32_t timeoutId )
{
    if ( isFinalized_ )
    {
        return;
    }

    HostTimerDispatcher::Get().killTimer( timeoutId );
}

JSObject* JsWindow::CreatePopupMenu()
{
    if ( isFinalized_ )
    {
        return nullptr;
    }

    return JsMenuObject::CreateJs( pJsCtx_, parentPanel_.GetHWND() );
}

JSObject* JsWindow::CreateThemeManager( const std::wstring& classid )
{
    if ( isFinalized_ )
    {
        return nullptr;
    }

    if ( !JsThemeManager::HasThemeData( parentPanel_.GetHWND(), classid ) )
    { // Not a error: not found
        return nullptr;
    }

    return JsThemeManager::CreateJs( pJsCtx_, parentPanel_.GetHWND(), classid );
}

JSObject* JsWindow::CreateTooltip( const std::wstring& name, uint32_t pxSize, uint32_t style )
{
    if ( isFinalized_ )
    {
        return nullptr;
    }

    if ( !jsTooltip_.initialized() )
    {
        jsTooltip_.init( pJsCtx_, JsFbTooltip::CreateJs( pJsCtx_, parentPanel_.GetHWND() ) );
        pNativeTooltip_ = static_cast<JsFbTooltip*>( JS_GetPrivate( jsTooltip_ ) );
    }

    assert( pNativeTooltip_ );
    pNativeTooltip_->SetFont( name, pxSize, style );
    
    return jsTooltip_;
}

JSObject* JsWindow::CreateTooltipWithOpt( size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style )
{
    switch ( optArgCount )
    {
    case 0:
        return CreateTooltip( name, pxSize, style );
    case 1:
        return CreateTooltip( name, pxSize );
    case 2:
        return CreateTooltip( name );
    case 3:
        return CreateTooltip();
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::DefinePanel( const std::u8string& name, JS::HandleValue options )
{
    if ( isFinalized_ )
    {
        return;
    }

    SmpException::ExpectTrue( !isPanelDefined_, "DefinePanel can't be called twice" );

    const auto parsedOptions = ParseDefinePanelOptions( options );

    parentPanel_.ScriptInfo().name = name;
    parentPanel_.ScriptInfo().author = parsedOptions.author;
    parentPanel_.ScriptInfo().version = parsedOptions.version;
    if ( parsedOptions.features.drag_n_drop )
    {
        dropTargetHandler_.Attach( new com_object_impl_t<com::HostDropTarget>( parentPanel_.GetHWND() ) );

        HRESULT hr = dropTargetHandler_->RegisterDragDrop();
        smp::error::CheckHR( hr, "RegisterDragDrop" );
    }

    isPanelDefined_ = true;
}

void JsWindow::DefinePanelWithOpt( size_t optArgCount, const std::u8string& name, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return DefinePanel( name, options );
    case 1:
        return DefinePanel( name );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsWindow::GetColourCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( isFinalized_ )
    {
        return 0;
    }

    SmpException::ExpectTrue( parentPanel_.GetPanelType() == panel::PanelType::CUI, "Can be called only in CUI" );

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        smp::error::CheckHR( hr, "CLSIDFromString" );
    }

    return parentPanel_.GetColour( type, guid );
}

uint32_t JsWindow::GetColourCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr )
{
    switch ( optArgCount )
    {
    case 0:
        return GetColourCUI( type, guidstr );
    case 1:
        return GetColourCUI( type );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsWindow::GetColourDUI( uint32_t type )
{
    if ( isFinalized_ )
    {
        return 0;
    }

    SmpException::ExpectTrue( parentPanel_.GetPanelType() == panel::PanelType::DUI, "Can be called only in DUI" );

    return parentPanel_.GetColour( type, pfc::guid_null );
}

JSObject* JsWindow::GetFontCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( isFinalized_ )
    {
        return nullptr;
    }

    SmpException::ExpectTrue( parentPanel_.GetPanelType() == panel::PanelType::CUI, "Can be called only in CUI" );

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        smp::error::CheckHR( hr, "CLSIDFromString" );
    }

    auto hFont = gdi::CreateUniquePtr( parentPanel_.GetFont( type, guid ) );
    if ( !hFont )
    { // Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont.get() ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont ) )
    { // Not an error: font not found
        return nullptr;
    }

    return JsGdiFont::CreateJs( pJsCtx_, std::move( pGdiFont ), hFont.release(), true );
}

JSObject* JsWindow::GetFontCUIWithOpt( size_t optArgCount, uint32_t type, const std::wstring& guidstr )
{
    switch ( optArgCount )
    {
    case 0:
        return GetFontCUI( type, guidstr );
    case 1:
        return GetFontCUI( type );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

JSObject* JsWindow::GetFontDUI( uint32_t type )
{
    if ( isFinalized_ )
    {
        return nullptr;
    }

    SmpException::ExpectTrue( parentPanel_.GetPanelType() == panel::PanelType::DUI, "Can be called only in DUI" );

    HFONT hFont = parentPanel_.GetFont( type, pfc::guid_null ); // No need to delete, it is managed by DUI
    if ( !hFont )
    { // Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont ) )
    { // Not an error: font not found
        return nullptr;
    }

    return JsGdiFont::CreateJs( pJsCtx_, std::move( pGdiFont ), hFont, false );
}

JS::Value JsWindow::GetProperty( const std::wstring& name, JS::HandleValue defaultval )
{
    if ( isFinalized_ )
    {
        return JS::UndefinedValue();
    }

    return fbProperties_->GetProperty( name, defaultval );
}

JS::Value JsWindow::GetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue defaultval )
{
    switch ( optArgCount )
    {
    case 0:
        return GetProperty( name, defaultval );
    case 1:
        return GetProperty( name );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::NotifyOthers( const std::wstring& name, JS::HandleValue info )
{
    if ( isFinalized_ )
    {
        return;
    }

    // TODO: think about replacing with PostMessage
    panel::message_manager::instance().send_msg_to_others(
        parentPanel_.GetHWND(),
        static_cast<UINT>( InternalSyncMessage::notify_data ),
        reinterpret_cast<WPARAM>( &name ),
        reinterpret_cast<LPARAM>( &info ) );
}

void JsWindow::Reload()
{
    if ( isFinalized_ )
    {
        return;
    }

    panel::message_manager::instance().post_msg( parentPanel_.GetHWND(), static_cast<UINT>( InternalAsyncMessage::reload_script ) );
}

void JsWindow::Repaint( bool force )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.Repaint( force );
}

void JsWindow::RepaintWithOpt( size_t optArgCount, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return Repaint( force );
    case 1:
        return Repaint();
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.RepaintRect( CRect{ static_cast<int>( x ), static_cast<int>( y ), static_cast<int>( x + w ), static_cast<int>( y + h ) }, force );
}

void JsWindow::RepaintRectWithOpt( size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return RepaintRect( x, y, w, h, force );
    case 1:
        return RepaintRect( x, y, w, h );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::SetCursor( uint32_t id )
{
    if ( isFinalized_ )
    {
        return;
    }

    ::SetCursor( LoadCursor( nullptr, MAKEINTRESOURCE( id ) ) );
}

uint32_t JsWindow::SetInterval( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    if ( isFinalized_ )
    {
        return 0;
    }

    SmpException::ExpectTrue( func.isObject() && JS_ObjectIsFunction( &func.toObject() ),
                              "func argument is not a JS function" );

    JS::RootedFunction jsFunction( pJsCtx_, JS_ValueToFunction( pJsCtx_, func ) );
    return HostTimerDispatcher::Get().setInterval( parentPanel_.GetHWND(), delay, pJsCtx_, jsFunction, funcArgs );
}

uint32_t JsWindow::SetIntervalWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    switch ( optArgCount )
    {
    case 0:
        return SetInterval( func, delay, funcArgs );
    case 1:
        return SetInterval( func, delay );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::SetProperty( const std::wstring& name, JS::HandleValue val )
{
    if ( isFinalized_ )
    {
        return;
    }

    fbProperties_->SetProperty( name, val );
}

void JsWindow::SetPropertyWithOpt( size_t optArgCount, const std::wstring& name, JS::HandleValue val )
{
    switch ( optArgCount )
    {
    case 0:
        return SetProperty( name, val );
    case 1:
        return SetProperty( name );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

uint32_t JsWindow::SetTimeout( JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    if ( isFinalized_ )
    {
        return 0;
    }

    SmpException::ExpectTrue( func.isObject() && JS_ObjectIsFunction( &func.toObject() ),
                              "func argument is not a JS function" );

    JS::RootedFunction jsFunction( pJsCtx_, JS_ValueToFunction( pJsCtx_, func ) );
    return HostTimerDispatcher::Get().setTimeout( parentPanel_.GetHWND(), delay, pJsCtx_, jsFunction, funcArgs );
}

uint32_t JsWindow::SetTimeoutWithOpt( size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs )
{
    switch ( optArgCount )
    {
    case 0:
        return SetTimeout( func, delay, funcArgs );
    case 1:
        return SetTimeout( func, delay );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsWindow::ShowConfigure()
{
    if ( isFinalized_ )
    {
        return;
    }

    panel::message_manager::instance().post_msg( parentPanel_.GetHWND(), static_cast<UINT>( InternalAsyncMessage::show_configure ) );
}

void JsWindow::ShowProperties()
{
    if ( isFinalized_ )
    {
        return;
    }

    panel::message_manager::instance().post_msg( parentPanel_.GetHWND(), static_cast<UINT>( InternalAsyncMessage::show_properties ) );
}

uint32_t JsWindow::get_DlgCode()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.DlgCode();
}

uint32_t JsWindow::get_Height()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.GetHeight();
}

uint32_t JsWindow::get_ID()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    // Such cast works properly only on x86
    return reinterpret_cast<uint32_t>( parentPanel_.GetHWND() );
}

uint32_t JsWindow::get_InstanceType()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return static_cast<uint32_t>( parentPanel_.GetPanelType() );
}

bool JsWindow::get_IsTransparent()
{
    if ( isFinalized_ )
    {
        return false;
    }

    return parentPanel_.GetSettings().isPseudoTransparent;
}

bool JsWindow::get_IsVisible()
{
    if ( isFinalized_ )
    {
        return false;
    }

    return IsWindowVisible( parentPanel_.GetHWND() );
}

uint32_t JsWindow::get_MaxHeight()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.MaxSize().y;
}

uint32_t JsWindow::get_MaxWidth()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.MaxSize().x;
}

uint32_t JsWindow::get_MemoryLimit()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return JsGc::GetMaxHeap();
}

uint32_t JsWindow::get_MinHeight()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.MinSize().y;
}

uint32_t JsWindow::get_MinWidth()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.MinSize().x;
}

std::u8string JsWindow::get_Name()
{
    if ( isFinalized_ )
    {
        return std::u8string{};
    }

    if ( const auto& name = parentPanel_.ScriptInfo().name;
         !name.empty() )
    {
        return name;
    }
    else
    {
        return pfc::print_guid( parentPanel_.GetGUID() ).get_ptr();
    }
}

uint64_t JsWindow::get_PanelMemoryUsage()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    JS::RootedObject jsGlobal( pJsCtx_, JS::CurrentGlobalOrNull( pJsCtx_ ) );
    return JsGc::GetTotalHeapUsageForGlobal( pJsCtx_, jsGlobal );
}

uint64_t JsWindow::get_TotalMemoryUsage()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return JsEngine::GetInstance().GetGcEngine().GetTotalHeapUsage();
}

JSObject* JsWindow::get_Tooltip()
{
    return CreateTooltip();
}

uint32_t JsWindow::get_Width()
{
    if ( isFinalized_ )
    {
        return 0;
    }

    return parentPanel_.GetWidth();
}

void JsWindow::put_DlgCode( uint32_t code )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.DlgCode() = code;
}

void JsWindow::put_MaxHeight( uint32_t height )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.MaxSize().y = height;
    PostMessage( parentPanel_.GetHWND(), static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_maximum_height, 0 );
}

void JsWindow::put_MaxWidth( uint32_t width )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.MaxSize().x = width;
    PostMessage( parentPanel_.GetHWND(), static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_maximum_width, 0 );
}

void JsWindow::put_MinHeight( uint32_t height )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.MinSize().y = height;
    PostMessage( parentPanel_.GetHWND(), static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_minimum_height, 0 );
}

void JsWindow::put_MinWidth( uint32_t width )
{
    if ( isFinalized_ )
    {
        return;
    }

    parentPanel_.MinSize().x = width;
    PostMessage( parentPanel_.GetHWND(), static_cast<UINT>( MiscMessage::size_limit_changed ), uie::size_limit_minimum_width, 0 );
}

JsWindow::DefinePanelOptions JsWindow::ParseDefinePanelOptions( JS::HandleValue options )
{
    DefinePanelOptions parsedOptions;
    if ( !options.isNullOrUndefined() )
    {
        SmpException::ExpectTrue( options.isObject(), "options argument is not an object" );
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );

        parsedOptions.author = GetOptionalProperty<std::u8string>( pJsCtx_, jsOptions, "author" ).value_or( "" );
        parsedOptions.version = GetOptionalProperty<std::u8string>( pJsCtx_, jsOptions, "version" ).value_or( "" );

        bool hasProperty;
        if ( !JS_HasProperty( pJsCtx_, jsOptions, "features", &hasProperty ) )
        {
            throw JsException();
        }

        if ( hasProperty )
        {
            JS::RootedValue jsFeaturesValue( pJsCtx_ );
            if ( !JS_GetProperty( pJsCtx_, jsOptions, "features", &jsFeaturesValue ) )
            {
                throw JsException();
            }

            SmpException::ExpectTrue( jsFeaturesValue.isObject(), "`features` is not an object" );

            JS::RootedObject jsFeatures( pJsCtx_, &jsFeaturesValue.toObject() );
            parsedOptions.features.drag_n_drop = GetOptionalProperty<bool>( pJsCtx_, jsFeatures, "drag_n_drop" ).value_or( false );
        }
    }

    return parsedOptions;
}

} // namespace mozjs
