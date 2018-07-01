#include <stdafx.h>

#include "fb_profiler.h"

#include <js_engine/js_to_native_invoker.h>
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
    JsFinalizeOp<JsFbProfiler>,
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

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbProfiler, Print )
MJS_DEFINE_JS_TO_NATIVE_FN( JsFbProfiler, Reset )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Print",  Print, 0, DefaultPropsFlags() ),
    JS_FN( "Reset",  Reset, 0, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsFbProfiler, get_Time )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Time", get_Time, DefaultPropsFlags() ),
    JS_PS_END
};

}

namespace mozjs
{


JsFbProfiler::JsFbProfiler( JSContext* cx, const std::string& name )
    : pJsCtx_( cx )
    , name_(name.c_str())
{
    timer_.start();
}

JsFbProfiler::~JsFbProfiler()
{
}

JSObject* JsFbProfiler::Create( JSContext* cx, const std::string& name )
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

    JS_SetPrivate( jsObj, new JsFbProfiler( cx, name ) );

    return jsObj;
}

const JSClass& JsFbProfiler::GetClass()
{
    return jsClass;
}

std::optional<std::nullptr_t> 
JsFbProfiler::Print()
{
    FB2K_console_formatter() 
        << JSP_NAME " v" JSP_VERSION ": FbProfiler (" << name_ << "): "
        << static_cast<uint32_t>(timer_.query() * 1000) << " ms";
    return nullptr;
}

std::optional<std::nullptr_t> 
JsFbProfiler::Reset()
{
    timer_.start();
    return nullptr;
}

std::optional<uint32_t> 
JsFbProfiler::get_Time()
{
    return static_cast<uint32_t>(timer_.query() * 1000);
}

}
