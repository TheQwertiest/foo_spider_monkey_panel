#include <stdafx.h>

#include "measure_string_info.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

namespace
{

using namespace mozjs;

constexpr JSClassOps jsOps = {
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

constexpr JSClass jsClass = {
    "MeasureStringInfo",
    kDefaultClassFlags,
    &jsOps
};

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Chars, JsMeasureStringInfo::get_Chars )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Height, JsMeasureStringInfo::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Lines, JsMeasureStringInfo::get_Lines )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Width, JsMeasureStringInfo::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_X, JsMeasureStringInfo::get_X )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_Y, JsMeasureStringInfo::get_Y )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "Chars", get_Chars, kDefaultPropsFlags ),
        JS_PSG( "Height", get_Height, kDefaultPropsFlags ),
        JS_PSG( "Lines", get_Lines, kDefaultPropsFlags ),
        JS_PSG( "Width", get_Width, kDefaultPropsFlags ),
        JS_PSG( "X", get_X, kDefaultPropsFlags ),
        JS_PSG( "Y", get_Y, kDefaultPropsFlags ),
        JS_PS_END,
    } );

} // namespace

namespace mozjs
{

const JSClass JsMeasureStringInfo::JsClass = jsClass;
const JSFunctionSpec* JsMeasureStringInfo::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsMeasureStringInfo::JsProperties = jsProperties.data();
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

uint32_t JsMeasureStringInfo::get_Chars() const
{
    return characters_;
}

float JsMeasureStringInfo::get_Height() const
{
    return h_;
}

uint32_t JsMeasureStringInfo::get_Lines() const
{
    return lines_;
}

float JsMeasureStringInfo::get_Width() const
{
    return w_;
}

float JsMeasureStringInfo::get_X() const
{
    return x_;
}

float JsMeasureStringInfo::get_Y() const
{
    return y_;
}

} // namespace mozjs
