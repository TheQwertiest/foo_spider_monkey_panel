#include <stdafx.h>
#include "window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/menu_object.h>
#include <js_objects/theme_manager.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_font.h>
#include <js_objects/internal/fb_properties.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <js_utils/scope_helper.h>
#include <js_utils/gdi_helpers.h>
#include <js_utils/winapi_error_helper.h>

#include <com_objects/host_drop_target.h>

#include <js_panel_window.h>
#include <panel_manager.h>
#include <helpers.h>
#include <user_message.h>

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
    nullptr
};

JSClass jsClass = {
    "Window",
    DefaultClassFlags(),
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
MJS_DEFINE_JS_FN_FROM_NATIVE( SetInterval, JsWindow::SetInterval )
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( SetProperty, JsWindow::SetProperty, JsWindow::SetPropertyWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetTimeout, JsWindow::SetTimeout )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowConfigure, JsWindow::ShowConfigure )
MJS_DEFINE_JS_FN_FROM_NATIVE( ShowProperties, JsWindow::ShowProperties )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "ClearInterval", ClearInterval, 1, DefaultPropsFlags() ),
    JS_FN( "ClearTimeout", ClearTimeout, 1, DefaultPropsFlags() ),
    JS_FN( "CreatePopupMenu", CreatePopupMenu, 0, DefaultPropsFlags() ),
    JS_FN( "CreateThemeManager", CreateThemeManager, 1, DefaultPropsFlags() ),
    JS_FN( "CreateTooltip", CreateTooltip, 0, DefaultPropsFlags() ),
    JS_FN( "DefinePanel", DefinePanel, 1, DefaultPropsFlags() ),
    JS_FN( "GetColourCUI", GetColourCUI, 1, DefaultPropsFlags() ),
    JS_FN( "GetColourDUI", GetColourDUI, 1, DefaultPropsFlags() ),
    JS_FN( "GetFontCUI", GetFontCUI, 1, DefaultPropsFlags() ),
    JS_FN( "GetFontDUI", GetFontDUI, 1, DefaultPropsFlags() ),
    JS_FN( "GetProperty", GetProperty, 1, DefaultPropsFlags() ),
    JS_FN( "NotifyOthers", NotifyOthers, 2, DefaultPropsFlags() ),
    JS_FN( "Reload", Reload, 0, DefaultPropsFlags() ),
    JS_FN( "Repaint", Repaint, 0, DefaultPropsFlags() ),
    JS_FN( "RepaintRect", RepaintRect, 4, DefaultPropsFlags() ),
    JS_FN( "SetCursor", SetCursor, 1, DefaultPropsFlags() ),
    JS_FN( "SetInterval", SetInterval, 2, DefaultPropsFlags() ),
    JS_FN( "SetProperty", SetProperty, 1, DefaultPropsFlags() ),
    JS_FN( "SetTimeout", SetTimeout, 2, DefaultPropsFlags() ),
    JS_FN( "ShowConfigure", ShowConfigure, 0, DefaultPropsFlags() ),
    JS_FN( "ShowProperties", ShowProperties, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_DlgCode, JsWindow::get_DlgCode )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsWindow::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_ID, JsWindow::get_ID )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_InstanceType, JsWindow::get_InstanceType )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsTransparent, JsWindow::get_IsTransparent )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_IsVisible, JsWindow::get_IsVisible )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MaxHeight, JsWindow::get_MaxHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MaxWidth, JsWindow::get_MaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MinHeight, JsWindow::get_MinHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_MinWidth, JsWindow::get_MinWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Name, JsWindow::get_Name )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsWindow::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_DlgCode, JsWindow::put_DlgCode )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MaxHeight, JsWindow::put_MaxHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MaxWidth, JsWindow::put_MaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MinHeight, JsWindow::put_MinHeight )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_MinWidth, JsWindow::put_MinWidth )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "DlgCode", get_DlgCode, put_DlgCode, DefaultPropsFlags() ),
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "ID", get_ID, DefaultPropsFlags() ),
    JS_PSG( "InstanceType", get_InstanceType, DefaultPropsFlags() ),
    JS_PSG( "IsTransparent", get_IsTransparent, DefaultPropsFlags() ),
    JS_PSG( "IsVisible", get_IsVisible, DefaultPropsFlags() ),
    JS_PSGS( "MaxHeight", get_MaxHeight, put_MaxHeight, DefaultPropsFlags() ),
    JS_PSGS( "MaxWidth", get_MaxWidth, put_MaxWidth, DefaultPropsFlags() ),
    JS_PSGS( "MinHeight", get_MinHeight, put_MinHeight, DefaultPropsFlags() ),
    JS_PSGS( "MinWidth", get_MinWidth, put_MinWidth, DefaultPropsFlags() ),
    JS_PSG( "Name", get_Name, DefaultPropsFlags() ),
    JS_PSG( "Width", get_Width, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsWindow::JsClass = jsClass;
const JSFunctionSpec* JsWindow::JsFunctions = jsFunctions;
const JSPropertySpec* JsWindow::JsProperties = jsProperties;

JsWindow::JsWindow( JSContext* cx, js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
    , fbProperties_( std::move( fbProperties ) )
{
}

JsWindow::~JsWindow()
{
    CleanupBeforeDestruction();
}

std::unique_ptr<JsWindow>
JsWindow::CreateNative( JSContext* cx, js_panel_window& parentPanel )
{
    std::unique_ptr<FbProperties> fbProperties = FbProperties::Create( cx, parentPanel );
    if ( !fbProperties )
    { // report in Create
        return nullptr;
    }

    return std::unique_ptr<JsWindow>( new JsWindow( cx, parentPanel, std::move( fbProperties ) ) );
}

size_t JsWindow::GetInternalSize( const js_panel_window& parentPanel )
{
    return sizeof( FbProperties );
}

void JsWindow::CleanupBeforeDestruction()
{
    if ( fbProperties_ )
    {
        fbProperties_->RemoveHeapTracer();
    }
    if ( dropTargetHandler_ )
    {
        dropTargetHandler_->RevokeDragDrop();
        dropTargetHandler_.Release();
    }
}

void JsWindow::ClearInterval( uint32_t intervalId )
{
    parentPanel_.ClearIntervalOrTimeout( intervalId );
}

void JsWindow::ClearTimeout( uint32_t timeoutId )
{
    parentPanel_.ClearIntervalOrTimeout( timeoutId );
}

JSObject* JsWindow::CreatePopupMenu()
{
    JS::RootedObject jsObject( pJsCtx_, JsMenuObject::CreateJs( pJsCtx_, parentPanel_.GetHWND() ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

JSObject* JsWindow::CreateThemeManager( const std::wstring& classid )
{
    if ( !JsThemeManager::HasThemeData( parentPanel_.GetHWND(), classid ) )
    { // Not a error: not found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsThemeManager::CreateJs( pJsCtx_, parentPanel_.GetHWND(), classid ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

JSObject* JsWindow::CreateTooltip( const std::wstring& name, float pxSize, uint32_t style )
{
    auto& tooltip_param = parentPanel_.GetPanelTooltipParam();
    tooltip_param.fontName = name;
    tooltip_param.fontSize = pxSize;
    tooltip_param.fontStyle = style;

    JS::RootedObject jsObject( pJsCtx_, JsFbTooltip::CreateJs( pJsCtx_, parentPanel_.GetHWND(), tooltip_param ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

JSObject* JsWindow::CreateTooltipWithOpt( size_t optArgCount, const std::wstring& name, float pxSize, uint32_t style )
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsWindow::DefinePanel( const pfc::string8_fast& name, JS::HandleValue options )
{ // TODO: clean up this mess
    if ( isPanelDefined_ )
    {
        throw smp::SmpException( "DefinePanel can't be called twice" );
    }

    struct Options
    {
        pfc::string8_fast author;
        pfc::string8_fast version;
        struct Features
        {
            bool drag_n_drop = false;
        } features;
    };

    Options parsed_options;
    {
        if ( !options.isNullOrUndefined() )
        {
            if ( !options.isObject() )
            {
                throw smp::SmpException( "options argument is not an object" );
            }

            JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );

            parsed_options.author = GetOptionalProperty<pfc::string8_fast>( pJsCtx_, jsOptions, "author" ).value_or( "" );
            parsed_options.version = GetOptionalProperty<pfc::string8_fast>( pJsCtx_, jsOptions, "version" ).value_or( "" );

            bool hasProperty;
            if ( !JS_HasProperty( pJsCtx_, jsOptions, "features", &hasProperty ) )
            {
                throw smp::JsException();
            }

            if ( hasProperty )
            {
                JS::RootedValue jsFeaturesValue( pJsCtx_ );
                if ( !JS_GetProperty( pJsCtx_, jsOptions, "features", &jsFeaturesValue ) )
                {
                    throw smp::JsException();
                }

                if ( !jsFeaturesValue.isObject() )
                {
                    throw smp::SmpException( "`features` is not an object" );
                }

                JS::RootedObject jsFeatures( pJsCtx_, &jsFeaturesValue.toObject() );
                parsed_options.features.drag_n_drop = GetOptionalProperty<bool>( pJsCtx_, jsFeatures, "drag_n_drop" ).value_or( false );
            }
        }
    }

    parentPanel_.ScriptInfo().name = name;
    parentPanel_.ScriptInfo().author = parsed_options.author;
    parentPanel_.ScriptInfo().version = parsed_options.version;
    if ( parsed_options.features.drag_n_drop )
    {
        dropTargetHandler_.Attach( new com_object_impl_t<HostDropTarget>( parentPanel_.GetHWND() ) );

        HRESULT hr = dropTargetHandler_->RegisterDragDrop();
        IF_HR_FAILED_THROW_SMP( hr, "RegisterDragDrop" );
    }

    isPanelDefined_ = true;
}

void JsWindow::DefinePanelWithOpt( size_t optArgCount, const pfc::string8_fast& name, JS::HandleValue options )
{
    switch ( optArgCount )
    {
    case 0:
        return DefinePanel( name, options );
    case 1:
        return DefinePanel( name );
    default:
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

uint32_t JsWindow::GetColourCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::CUI )
    {
        throw smp::SmpException( "Can be called only in CUI" );
    }

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        IF_HR_FAILED_THROW_SMP( hr, "CLSIDFromString" );
    }

    return parentPanel_.GetColourCUI( type, guid );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

uint32_t JsWindow::GetColourDUI( uint32_t type )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::DUI )
    {
        throw smp::SmpException( "Can be called only in DUI" );
    }

    return parentPanel_.GetColourDUI( type );
}

JSObject* JsWindow::GetFontCUI( uint32_t type, const std::wstring& guidstr )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::CUI )
    {
        throw smp::SmpException( "Can be called only in CUI" );
    }

    GUID guid;
    if ( guidstr.empty() )
    {
        memcpy( &guid, &pfc::guid_null, sizeof( guid ) );
    }
    else
    {
        HRESULT hr = CLSIDFromString( guidstr.c_str(), &guid );
        IF_HR_FAILED_THROW_SMP( hr, "CLSIDFromString" );
    }

    auto hFont = gdi::CreateUniquePtr( parentPanel_.GetFontCUI( type, guid ) );
    if ( !hFont )
    { // Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont.get() ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont.get() ) )
    { // Not an error: font not found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiFont::CreateJs( pJsCtx_, std::move( pGdiFont ), hFont.release(), true ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    hFont.release();
    return jsObject;
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

JSObject* JsWindow::GetFontDUI( uint32_t type )
{
    if ( parentPanel_.GetPanelType() != js_panel_window::PanelType::DUI )
    {
        throw smp::SmpException( "Can be called only in DUI" );
    }

    HFONT hFont = parentPanel_.GetFontDUI( type ); // No need to delete, it is managed by DUI
    if ( !hFont )
    { // Not an error: font not found
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Font> pGdiFont( new Gdiplus::Font( parentPanel_.GetHDC(), hFont ) );
    if ( !gdi::IsGdiPlusObjectValid( pGdiFont.get() ) )
    { // Not an error: font not found
        return nullptr;
    }

    JS::RootedObject jsObject( pJsCtx_, JsGdiFont::CreateJs( pJsCtx_, std::move( pGdiFont ), hFont, false ) );
    if ( !jsObject )
    { // TODO: remove
        throw smp::JsException();
    }

    return jsObject;
}

JS::Value JsWindow::GetProperty( const std::wstring& name, JS::HandleValue defaultval )
{
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsWindow::NotifyOthers( const std::wstring& name, JS::HandleValue info )
{
    // TODO: think about replacing with PostMessage
    panel_manager::instance().send_msg_to_others(
        parentPanel_.GetHWND(),
        CALLBACK_UWM_ON_NOTIFY_DATA,
        reinterpret_cast<WPARAM>( &name ),
        reinterpret_cast<LPARAM>( &info ) );
}

void JsWindow::Reload()
{
    PostMessage( parentPanel_.GetHWND(), UWM_RELOAD, 0, 0 );
}

void JsWindow::Repaint( bool force )
{
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsWindow::RepaintRect( uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force )
{
    parentPanel_.RepaintRect( x, y, w, h, force );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

void JsWindow::SetCursor( uint32_t id )
{
    ::SetCursor( LoadCursor( nullptr, MAKEINTRESOURCE( id ) ) );
}

uint32_t JsWindow::SetInterval( JS::HandleValue func, uint32_t delay )
{ // TODO: try to remove the roundabout call (JsWindow > js_panel_window > JsContainer)
    if ( !func.isObject() || !JS_ObjectIsFunction( pJsCtx_, &func.toObject() ) )
    {
        throw smp::SmpException( "func argument is not a JS function" );
    }

    JS::RootedFunction jsFunction( pJsCtx_, JS_ValueToFunction( pJsCtx_, func ) );
    return parentPanel_.SetInterval( jsFunction, delay );
}

void JsWindow::SetProperty( const std::wstring& name, JS::HandleValue val )
{
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

uint32_t JsWindow::SetTimeout( JS::HandleValue func, uint32_t delay )
{
    if ( !func.isObject() || !JS_ObjectIsFunction( pJsCtx_, &func.toObject() ) )
    {
        throw smp::SmpException( "func argument is not a JS function" );
    }

    JS::RootedFunction jsFunction( pJsCtx_, JS_ValueToFunction( pJsCtx_, func ) );
    return parentPanel_.SetTimeout( jsFunction, delay );
}

void JsWindow::ShowConfigure()
{
    PostMessage( parentPanel_.GetHWND(), UWM_SHOW_CONFIGURE, 0, 0 );
}

void JsWindow::ShowProperties()
{
    PostMessage( parentPanel_.GetHWND(), UWM_SHOW_PROPERTIES, 0, 0 );
}

uint32_t JsWindow::get_DlgCode()
{
    return parentPanel_.DlgCode();
}

uint32_t JsWindow::get_Height()
{
    return parentPanel_.GetHeight();
}

uint32_t JsWindow::get_ID()
{ // Will work properly only on x86
    return reinterpret_cast<uint32_t>( parentPanel_.GetHWND() );
}

uint32_t JsWindow::get_InstanceType()
{
    return static_cast<uint32_t>( parentPanel_.GetPanelType() );
}

bool JsWindow::get_IsTransparent()
{
    return parentPanel_.get_pseudo_transparent();
}

bool JsWindow::get_IsVisible()
{
    return IsWindowVisible( parentPanel_.GetHWND() );
}

uint32_t JsWindow::get_MaxHeight()
{
    return parentPanel_.MaxSize().y;
}

uint32_t JsWindow::get_MaxWidth()
{
    return parentPanel_.MaxSize().x;
}

uint32_t JsWindow::get_MinHeight()
{
    return parentPanel_.MinSize().y;
}

uint32_t JsWindow::get_MinWidth()
{
    return parentPanel_.MinSize().x;
}

pfc::string8_fast JsWindow::get_Name()
{
    pfc::string8_fast name = parentPanel_.ScriptInfo().name;
    if ( name.is_empty() )
    {
        name = pfc::print_guid( parentPanel_.GetGUID() );
    }

    return pfc::string8_fast( name.c_str(), name.length() );
}

uint32_t JsWindow::get_Width()
{
    return parentPanel_.GetWidth();
}

void JsWindow::put_DlgCode( uint32_t code )
{
    parentPanel_.DlgCode() = code;
}

void JsWindow::put_MaxHeight( uint32_t height )
{
    parentPanel_.MaxSize().y = height;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_height );
}

void JsWindow::put_MaxWidth( uint32_t width )
{
    parentPanel_.MaxSize().x = width;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_maximum_width );
}

void JsWindow::put_MinHeight( uint32_t height )
{
    parentPanel_.MinSize().y = height;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_height );
}

void JsWindow::put_MinWidth( uint32_t width )
{
    parentPanel_.MinSize().x = width;
    PostMessage( parentPanel_.GetHWND(), UWM_SIZE_LIMIT_CHANGED, 0, uie::size_limit_minimum_width );
}

} // namespace mozjs
