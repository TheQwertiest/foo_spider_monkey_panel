#include <stdafx.h>
#include "fb_title_format.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/string_helpers.h>

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
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Eval, JsFbTitleFormat::Eval, JsFbTitleFormat::EvalWithOpt, 1 )
MJS_DEFINE_JS_FN_FROM_NATIVE( EvalWithMetadb, JsFbTitleFormat::EvalWithMetadb )
MJS_DEFINE_JS_FN_FROM_NATIVE( EvalWithMetadbs, JsFbTitleFormat::EvalWithMetadbs )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Eval", Eval, 0, DefaultPropsFlags() ),
    JS_FN( "EvalWithMetadb", EvalWithMetadb, 1, DefaultPropsFlags() ),
    JS_FN( "EvalWithMetadbs", EvalWithMetadbs, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsFbTitleFormat::JsClass = jsClass;
const JSFunctionSpec* JsFbTitleFormat::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbTitleFormat::JsProperties = jsProperties;
const JsPrototypeId JsFbTitleFormat::PrototypeId = JsPrototypeId::FbTitleFormat;

JsFbTitleFormat::JsFbTitleFormat( JSContext* cx, const pfc::string8_fast& expr )
    : pJsCtx_( cx )
{
    titleformat_compiler::get()->compile_safe( titleFormatObject_, expr.c_str() );
}

JsFbTitleFormat::~JsFbTitleFormat()
{
}

std::unique_ptr<JsFbTitleFormat>
JsFbTitleFormat::CreateNative( JSContext* cx, const pfc::string8_fast& expr )
{
    return std::unique_ptr<JsFbTitleFormat>( new JsFbTitleFormat( cx, expr ) );
}

size_t JsFbTitleFormat::GetInternalSize( const pfc::string8_fast& /*expr*/ )
{
    return sizeof( titleformat_object );
}

titleformat_object::ptr JsFbTitleFormat::GetTitleFormat()
{
    return titleFormatObject_;
}

pfc::string8_fast JsFbTitleFormat::Eval( bool force )
{
    auto pc = playback_control::get();
    metadb_handle_ptr handle;
    pfc::string8_fast text;

    if ( !pc->is_playing() && force )
    { // Trying to get handle to any known playable location
        if ( !metadb::g_get_random_handle( handle ) )
        { // Fake handle, workaround recommended by foobar2000 devs
            playable_location_impl dummy;
            metadb::get()->handle_create( handle, dummy );
        }
    }
    pc->playback_format_title_ex( handle, nullptr, text, titleFormatObject_, nullptr, playback_control::display_level_all );

    return pfc::string8_fast( text.c_str(), text.length() );
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
        throw smp::SmpException( smp::string::Formatter() << "Internal error: invalid number of optional arguments specified: " << optArgCount );
    }
}

pfc::string8_fast JsFbTitleFormat::EvalWithMetadb( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        throw smp::SmpException( "handle argument is null" );
    }

    pfc::string8_fast text;
    handle->GetHandle()->format_title( nullptr, text, titleFormatObject_, nullptr );

    return pfc::string8_fast( text.c_str(), text.length() );
}

JSObject* JsFbTitleFormat::EvalWithMetadbs( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        throw smp::SmpException( "handles argument is null" );
    }

    metadb_handle_list_cref handles_cref = handles->GetHandleList();
    t_size count = handles_cref.get_count();

    JS::RootedObject evalResult( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !evalResult )
    {
        throw smp::JsException();
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        pfc::string8_fast text;
        handles_cref[i]->format_title( nullptr, text, titleFormatObject_, nullptr );

        if ( !convert::to_js::ToValue( pJsCtx_, text, &jsValue ) )
        {
            throw smp::SmpException( "Internal error: cast to JSString failed" );
        }

        if ( !JS_SetElement( pJsCtx_, evalResult, i, jsValue ) )
        {
            throw smp::JsException();
        }
    }

    return evalResult;
}

} // namespace mozjs
