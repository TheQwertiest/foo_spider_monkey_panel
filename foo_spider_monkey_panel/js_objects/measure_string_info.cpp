#include <stdafx.h>
#include "measure_string_info.h"

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
    JsMeasureStringInfo::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "MeasureStringInfo",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Chars, JsMeasureStringInfo::get_Chars )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsMeasureStringInfo::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Lines, JsMeasureStringInfo::get_Lines )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsMeasureStringInfo::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_X, JsMeasureStringInfo::get_X )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Y, JsMeasureStringInfo::get_Y )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Chars", get_Chars, DefaultPropsFlags() ),
    JS_PSG( "Height", get_Height, DefaultPropsFlags() ),
    JS_PSG( "Lines", get_Lines, DefaultPropsFlags() ),
    JS_PSG( "Width", get_Width, DefaultPropsFlags() ),
    JS_PSG( "X", get_X, DefaultPropsFlags() ),
    JS_PSG( "Y", get_Y, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsMeasureStringInfo::JsClass = jsClass;
const JSFunctionSpec* JsMeasureStringInfo::JsFunctions = jsFunctions;
const JSPropertySpec* JsMeasureStringInfo::JsProperties = jsProperties;
const JsPrototypeId JsMeasureStringInfo::PrototypeId = JsPrototypeId::MeasureStringInfo;

JsMeasureStringInfo::JsMeasureStringInfo( JSContext* cx, float x, float y, float w, float h, uint32_t lines, uint32_t characters )
    : pJsCtx_( cx )
    , x_( x )
    , y_( y )
    , w_( w )
    , h_( h )
    , lines_( lines )
    , characters_( characters )
{
}

std::unique_ptr<JsMeasureStringInfo>
JsMeasureStringInfo::CreateNative( JSContext* cx, float x, float y, float w, float h, uint32_t lines, uint32_t characters )
{
    return std::unique_ptr<JsMeasureStringInfo>( new JsMeasureStringInfo( cx, x, y, w, h, lines, characters ) );
}

size_t JsMeasureStringInfo::GetInternalSize( float /*x*/, float /*y*/, float /*w*/, float /*h*/, uint32_t /*l*/, uint32_t /*c*/ )
{
    return 0;
}

uint32_t JsMeasureStringInfo::get_Chars()
{
    return characters_;
}

float JsMeasureStringInfo::get_Height()
{
    return h_;
}

uint32_t JsMeasureStringInfo::get_Lines()
{
    return lines_;
}

float JsMeasureStringInfo::get_Width()
{
    return w_;
}

float JsMeasureStringInfo::get_X()
{
    return x_;
}

float JsMeasureStringInfo::get_Y()
{
    return y_;
}

} // namespace mozjs
