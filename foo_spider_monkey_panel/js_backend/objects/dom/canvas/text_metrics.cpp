#include <stdafx.h>

#include "text_metrics.h"

#include <js_backend/engine/js_to_native_invoker.h>

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
    TextMetrics::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "TextMetrics",
    kDefaultClassFlags,
    &jsOps
};

// MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ActualBoundingBoxAscent, TextMetrics::get_ActualBoundingBoxAscent )
// MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ActualBoundingBoxDescent, TextMetrics::get_ActualBoundingBoxDescent )
// MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ActualBoundingBoxLeft, TextMetrics::get_ActualBoundingBoxLeft )
// MJS_DEFINE_JS_FN_FROM_NATIVE( Get_ActualBoundingBoxRight, TextMetrics::get_ActualBoundingBoxRight )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, TextMetrics::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, TextMetrics::get_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        // TODO: implement
        // JS_PSG( "actualBoundingBoxAscent", Get_ActualBoundingBoxAscent, kDefaultPropsFlags ),
        // JS_PSG( "actualBoundingBoxDescent", Get_ActualBoundingBoxDescent, kDefaultPropsFlags ),
        // JS_PSG( "actualBoundingBoxLeft", Get_ActualBoundingBoxLeft, kDefaultPropsFlags ),
        // JS_PSG( "actualBoundingBoxRight", Get_ActualBoundingBoxRight, kDefaultPropsFlags ),
        JS_PSG( "height", get_height, kDefaultPropsFlags ),
        JS_PSG( "width", get_width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::TextMetrics );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<TextMetrics>::JsClass = jsClass;
const JSPropertySpec* JsObjectTraits<TextMetrics>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<TextMetrics>::PrototypeId = JsPrototypeId::New_TextMetrics;

TextMetrics::TextMetrics( JSContext* cx, const MetricsData& data )
    : pJsCtx_( cx )
    , data_{ data }
{
}

TextMetrics::~TextMetrics()
{
}

std::unique_ptr<TextMetrics>
TextMetrics::CreateNative( JSContext* cx, const MetricsData& data )
{
    return std::unique_ptr<TextMetrics>( new TextMetrics( cx, data ) );
}

size_t TextMetrics::GetInternalSize() const
{
    return 0;
}

double TextMetrics::get_ActualBoundingBoxAscent() const
{
    return data_.actualBoundingBoxAscent;
}

double TextMetrics::get_ActualBoundingBoxDescent() const
{
    return data_.actualBoundingBoxDescent;
}

double TextMetrics::get_ActualBoundingBoxLeft() const
{
    return data_.actualBoundingBoxLeft;
}

double TextMetrics::get_ActualBoundingBoxRight() const
{
    return data_.actualBoundingBoxRight;
}

double TextMetrics::get_Height() const
{
    return data_.height;
}

double TextMetrics::get_Width() const
{
    return data_.width;
}

} // namespace mozjs
