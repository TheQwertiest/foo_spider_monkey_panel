#include <stdafx.h>
#include "fb_title_format.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


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
    JsFinalizeOp<JsFbTitleFormat>,
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

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTitleFormat, Eval )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTitleFormat, EvalWithMetadb )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbTitleFormat, EvalWithMetadbs )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Eval", Eval, 1, DefaultPropsFlags() ),
    JS_FN( "EvalWithMetadb", EvalWithMetadb, 1, DefaultPropsFlags() ),
    JS_FN( "EvalWithMetadbs", EvalWithMetadbs, 1, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

JsFbTitleFormat::JsFbTitleFormat( JSContext* cx, const std::string& expr )
    : pJsCtx_( cx )
{
    titleformat_compiler::get()->compile_safe( titleFormatObject_, expr.c_str() );
}

JsFbTitleFormat::~JsFbTitleFormat()
{
}

JSObject* JsFbTitleFormat::Create( JSContext* cx, const std::string& expr )
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

    JS_SetPrivate( jsObj, new JsFbTitleFormat( cx, expr ) );

    return jsObj;
}

const JSClass& JsFbTitleFormat::GetClass()
{
    return jsClass;
}

std::optional<std::string> 
JsFbTitleFormat::Eval( bool force )
{
    auto pc = playback_control::get();
    metadb_handle_ptr handle;
    pfc::string8_fast text;

    if ( !pc->is_playing() && force )
    {// Trying to get handle to any known playable location
        if ( !metadb::g_get_random_handle( handle ) )
        {// Fake handle, workaround recommended by foobar2000 devs
            playable_location_impl dummy;
            metadb::get()->handle_create( handle, dummy );
        }
    }
    pc->playback_format_title_ex( handle, nullptr, text, titleFormatObject_, nullptr, playback_control::display_level_all );

    return std::string( text.c_str(), text.length() );
}

std::optional<std::string>
JsFbTitleFormat::EvalWithMetadb( JsFbMetadbHandle* handle )
{
    if ( !handle )
    {
        JS_ReportErrorASCII( pJsCtx_, "handle argument is null" );
        return std::nullopt;
    }

    pfc::string8_fast text;
    handle->GetHandle()->format_title( nullptr, text, titleFormatObject_, nullptr );

    return std::string( text.c_str(), text.length());
}

std::optional<JSObject*> 
JsFbTitleFormat::EvalWithMetadbs( JsFbMetadbHandleList* handles )
{
    if ( !handles )
    {
        JS_ReportErrorASCII( pJsCtx_, "handles argument is null" );
        return std::nullopt;
    }

    metadb_handle_list_cref handles_cref = handles->GetHandleList();
    t_size count = handles_cref.get_count();

    JS::RootedObject evalResult( pJsCtx_, JS_NewArrayObject( pJsCtx_, count ) );
    if ( !evalResult )
    {
        JS_ReportOutOfMemory( pJsCtx_ );
        return std::nullopt;
    }

    JS::RootedValue jsValue( pJsCtx_ );
    for ( t_size i = 0; i < count; ++i )
    {
        pfc::string8_fast text;
        handles_cref[i]->format_title( nullptr, text, titleFormatObject_, nullptr );

        std::string tmpString( text.c_str(), text.length() );
        if ( !convert::to_js::ToValue( pJsCtx_, tmpString, &jsValue ) )
        {
            JS_ReportErrorASCII( pJsCtx_, "Internal error: cast to JSString failed" );
            return std::nullopt;
        }

        if ( !JS_SetElement( pJsCtx_, evalResult, i, jsValue ) )
        {
            JS_ReportErrorASCII( pJsCtx_, "Internal error: JS_SetElement failed" );
            return std::nullopt;
        }
    }
    
    return evalResult;
}

}
