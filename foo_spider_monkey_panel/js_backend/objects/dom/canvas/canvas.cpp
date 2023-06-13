#include <stdafx.h>

#include "canvas.h"

#include <graphics/gdiplus/error_handler.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/canvas/canvas_rendering_context_2d.h>

#include <qwr/final_action.h>
#include <qwr/utility.h>

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
    Canvas::FinalizeJsObject,
    nullptr,
    nullptr,
    Canvas::Trace
};

JSClass jsClass = {
    "Canvas",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT_AND_SELF( getContext, Canvas::GetContext, Canvas::GetContextWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "getContext", getContext, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_height, mozjs::Canvas::get_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_width, mozjs::Canvas::get_Width )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_height, mozjs::Canvas::put_Height )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_width, mozjs::Canvas::put_Width )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSGS( "height", get_height, put_height, kDefaultPropsFlags ),
        JS_PSGS( "width", get_width, put_width, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( Canvas_Constructor, Canvas::Constructor )

MJS_VERIFY_OBJECT( mozjs::Canvas );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Canvas>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Canvas>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<Canvas>::JsProperties = jsProperties.data();
const JsPrototypeId JsObjectTraits<Canvas>::PrototypeId = JsPrototypeId::New_Canvas;
const JSNative JsObjectTraits<Canvas>::JsConstructor = ::Canvas_Constructor;

Canvas::Canvas( JSContext* cx, uint32_t width, uint32_t height )
    : pJsCtx_( cx )
{
    ReinitializeCanvas( width, height );
}

Canvas::~Canvas()
{
}

std::unique_ptr<Canvas>
Canvas::CreateNative( JSContext* cx, uint32_t width, uint32_t height )
{
    return std::unique_ptr<Canvas>( new Canvas( cx, width, height ) );
}

size_t Canvas::GetInternalSize() const
{
    return sizeof( Gdiplus::Bitmap ) + pBitmap_->GetWidth() * pBitmap_->GetHeight() * Gdiplus::GetPixelFormatSize( pBitmap_->GetPixelFormat() ) / 8;
}

void Canvas::Trace( JSTracer* trc, JSObject* obj )
{
    auto pNative = JsObjectBase<Canvas>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsRenderingContext_, "Heap: Canvas: rendering context" );
}

Gdiplus::Bitmap& Canvas::GetBitmap()
{
    assert( pBitmap_ );
    return *pBitmap_;
}

bool Canvas::IsDevice() const
{
    return false;
}

Gdiplus::Bitmap* Canvas::GetBmp()
{
    return pBitmap_.get();
}

Gdiplus::Graphics& Canvas::GetGraphics()
{
    assert( pGraphics_ );
    return *pGraphics_;
}

uint32_t Canvas::GetHeight() const
{
    return pBitmap_->GetHeight();
}

uint32_t Canvas::GetWidth() const
{
    return pBitmap_->GetWidth();
}

JSObject* Canvas::Constructor( JSContext* cx, uint32_t width, uint32_t height )
{
    return JsObjectBase<Canvas>::CreateJs( cx, width, height );
}

JSObject* Canvas::GetContext( JS::HandleObject jsSelf, const std::wstring& contextType, JS::HandleValue attributes )
{
    qwr::QwrException::ExpectTrue( contextType == L"2d", L"Unsupported contextType value: {}", contextType );

    // TODO: handle attributes
    if ( !jsRenderingContext_.get() )
    {
        jsRenderingContext_ = JsObjectBase<CanvasRenderingContext2D_Qwr>::CreateJs( pJsCtx_, jsSelf, *this );

        JS::RootedObject jsRootedRenderingContext( pJsCtx_, jsRenderingContext_.get() );
        pNativeRenderingContext_ = JsObjectBase<CanvasRenderingContext2D_Qwr>::ExtractNative( pJsCtx_, jsRootedRenderingContext );
        assert( pNativeRenderingContext_ );
    }

    return jsRenderingContext_.get();
}

JSObject* Canvas::GetContextWithOpt( JS::HandleObject jsSelf, size_t optArgCount, const std::wstring& contextType, JS::HandleValue attributes )
{
    switch ( optArgCount )
    {
    case 0:
        return GetContext( jsSelf, contextType, attributes );
    case 1:
        return GetContext( jsSelf, contextType );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

uint32_t Canvas::get_Height() const
{
    return GetHeight();
}

uint32_t Canvas::get_Width() const
{
    return GetWidth();
}

void Canvas::put_Height( uint32_t value )
{
    ReinitializeCanvas( pBitmap_->GetWidth(), value );
}

void Canvas::put_Width( uint32_t value )
{
    ReinitializeCanvas( value, pBitmap_->GetHeight() );
}

void Canvas::ReinitializeCanvas( uint32_t width, uint32_t height )
{
    if ( width <= 0 )
    {
        width = 300;
    }
    if ( height <= 0 )
    {
        height = 300;
    }

    auto pBitmap = std::make_unique<Gdiplus::Bitmap>( width, height, PixelFormat32bppPARGB );
    smp::CheckGdiPlusObject( pBitmap );

    auto pGraphics = std::make_unique<Gdiplus::Graphics>( pBitmap.get() );
    smp::CheckGdiPlusObject( pGraphics );

    pBitmap_ = std::move( pBitmap );
    pGraphics_ = std::move( pGraphics );

    if ( pNativeRenderingContext_ )
    {
        pNativeRenderingContext_->Reinitialize();
    }
}

} // namespace mozjs
