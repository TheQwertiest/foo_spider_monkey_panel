#include <stdafx.h>

#include "fb_profiler.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <smp_exception.h>

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
    JsFbProfiler::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbProfiler",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( Print, JsFbProfiler::Print, JsFbProfiler::PrintWithOpt, 2 )
MJS_DEFINE_JS_FN_FROM_NATIVE( Reset, JsFbProfiler::Reset )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Print", Print, 0, DefaultPropsFlags() ),
    JS_FN( "Reset", Reset, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Time, JsFbProfiler::get_Time )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Time", get_Time, DefaultPropsFlags() ),
    JS_PS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( FbProfiler_Constructor, JsFbProfiler::Constructor, JsFbProfiler::ConstructorWithOpt, 1 )

} // namespace

namespace mozjs
{

const JSClass JsFbProfiler::JsClass = jsClass;
const JSFunctionSpec* JsFbProfiler::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbProfiler::JsProperties = jsProperties;
const JsPrototypeId JsFbProfiler::PrototypeId = JsPrototypeId::FbProfiler;
const JSNative JsFbProfiler::JsConstructor = ::FbProfiler_Constructor;

JsFbProfiler::JsFbProfiler( JSContext* cx, const std::u8string& name )
    : pJsCtx_( cx )
    , name_( name )
{
    timer_.start();
}

JsFbProfiler::~JsFbProfiler()
{
}

std::unique_ptr<JsFbProfiler>
JsFbProfiler::CreateNative( JSContext* cx, const std::u8string& name )
{
    return std::unique_ptr<JsFbProfiler>( new JsFbProfiler( cx, name ) );
}

size_t JsFbProfiler::GetInternalSize( const std::u8string& name )
{
    return name.length();
}

JSObject* JsFbProfiler::Constructor( JSContext* cx, const std::u8string& name )
{
    return JsFbProfiler::CreateJs( cx, name );
}

JSObject* JsFbProfiler::ConstructorWithOpt( JSContext* cx, size_t optArgCount, const std::u8string& name )
{
    switch ( optArgCount )
    {
    case 0:
        return Constructor( cx, name );
    case 1:
        return Constructor( cx, name );
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbProfiler::Print( const std::u8string& additionalMsg, bool printComponentInfo )
{
    std::u8string msg;
    if ( printComponentInfo )
    {
        msg += SMP_NAME_WITH_VERSION ": ";
    }
    msg += "profiler";
    if ( !name_.empty() )
    {
        msg += fmt::format( " ({})", name_.c_str() );
    }
    msg += ":";
    if ( !additionalMsg.empty() )
    {
        msg += " " + additionalMsg;
    }
    msg += fmt::format( " {}ms", static_cast<uint32_t>( timer_.query() * 1000 ) );

    FB2K_console_formatter() << msg;
}

void JsFbProfiler::PrintWithOpt( size_t optArgCount, const std::u8string& additionalMsg, bool printComponentInfo )
{
    switch ( optArgCount )
    {
    case 0:
        return Print( additionalMsg, printComponentInfo );
    case 1:
        return Print( additionalMsg );
    case 2:
        return Print();
    default:
        throw SmpException( fmt::format( "Internal error: invalid number of optional arguments specified: {}", optArgCount ) );
    }
}

void JsFbProfiler::Reset()
{
    timer_.start();
}

uint32_t JsFbProfiler::get_Time()
{
    return static_cast<uint32_t>( timer_.query() * 1000 );
}

} // namespace mozjs
