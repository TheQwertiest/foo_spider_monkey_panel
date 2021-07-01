#include <stdafx.h>

#include "fb_title_format.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

#include <qwr/string_helpers.h>

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
    JsFbTitleFormat::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbTitleFormat",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Eval, JsFbTitleFormat::Eval, JsFbTitleFormat::EvalWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( EvalWithMetadb, JsFbTitleFormat::EvalWithMetadb )
MJS_DEFINE_JS_FN_FROM_NATIVE( EvalWithMetadbs, JsFbTitleFormat::EvalWithMetadbs )

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "Eval", Eval, 0, kDefaultPropsFlags ),
        JS_FN( "EvalWithMetadb", EvalWithMetadb, 1, kDefaultPropsFlags ),
        JS_FN( "EvalWithMetadbs", EvalWithMetadbs, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( FbTitleFormat_Constructor, JsFbTitleFormat::Constructor )

} // namespace

namespace mozjs
{

const JSClass JsFbTitleFormat::JsClass = jsClass;
const JSFunctionSpec* JsFbTitleFormat::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbTitleFormat::JsProperties = jsProperties.data();
const JsPrototypeId JsFbTitleFormat::PrototypeId = JsPrototypeId::FbTitleFormat;
const JSNative JsFbTitleFormat::JsConstructor = ::FbTitleFormat_Constructor;

JsFbTitleFormat::JsFbTitleFormat( JSContext* cx, const qwr::u8string& expr )
    : pJsCtx_( cx )
{
    titleformat_compiler::get()->compile_safe( titleFormatObject_, expr.c_str() );
}

std::unique_ptr<JsFbTitleFormat>
JsFbTitleFormat::CreateNative( JSContext* cx, const qwr::u8string& expr )
{
    return std::unique_ptr<JsFbTitleFormat>( new JsFbTitleFormat( cx, expr ) );
}

size_t JsFbTitleFormat::GetInternalSize( const qwr::u8string& /*expr*/ )
{
    return sizeof( titleformat_object );
}

titleformat_object::ptr JsFbTitleFormat::GetTitleFormat()
{
    return titleFormatObject_;
}

JSObject* JsFbTitleFormat::Constructor( JSContext* cx, const qwr::u8string& expr )
{
    return JsFbTitleFormat::CreateJs( cx, expr );
}

pfc::string8_fast JsFbTitleFormat::Eval( bool force )
{
    auto pc = playback_control::get();
    metadb_handle_ptr handle;

    if ( !pc->is_playing() && force )
    { // Trying to get handle to any known playable location
        if ( !metadb::g_get_random_handle( handle ) )
        { // Fake handle, workaround recommended by foobar2000 devs
            metadb::get()->handle_create( handle, playable_location_impl{} );
        }
    }

    pfc::string8_fast text;
    pc->playback_format_title_ex( handle, nullptr, text, titleFormatObject_, nullptr, playback_control::display_level_all );
    return text;
}

pfc::string8_fast JsFbTitleFormat::EvalWithOpt( size_t optArgCount, bool force )
{
    switch ( optArgCount )
    {
    case 0:
        return Eval( force );
    case 1:
        return Eval();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

pfc::string8_fast JsFbTitleFormat::EvalWithMetadb( JsFbMetadbHandle* handle )
{
    qwr::QwrException::ExpectTrue( handle, "handle argument is null" );

    pfc::string8_fast text;
    handle->GetHandle()->format_title( nullptr, text, titleFormatObject_, nullptr );
    return text;
}

JS::Value JsFbTitleFormat::EvalWithMetadbs( JsFbMetadbHandleList* handles )
{
    qwr::QwrException::ExpectTrue( handles, "handles argument is null" );

    JS::RootedValue jsValue( pJsCtx_ );
    convert::to_js::ToArrayValue(
        pJsCtx_,
        qwr::pfc_x::Make_Stl_CRef( handles->GetHandleList() ),
        [&titleFormat = titleFormatObject_]( const auto& vec, auto index ) {
            pfc::string8_fast text;
            vec[index]->format_title( nullptr, text, titleFormat, nullptr );
            return text;
        },
        &jsValue );
    return jsValue;
}

} // namespace mozjs
