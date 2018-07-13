#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <memory>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Bitmap;
}

namespace mozjs
{

class JsGdiGraphics;

class JsGdiBitmap
    : public JsObjectBase<JsGdiBitmap>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsGdiBitmap();

    static std::unique_ptr<JsGdiBitmap> CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap );

public:
    Gdiplus::Bitmap* GdiBitmap() const;

public: //methods
    std::optional<JSObject*> ApplyAlpha( uint8_t alpha );
    std::optional<bool> ApplyMask( JsGdiBitmap* mask );
    std::optional<JSObject*> Clone( float x, float y, float w, float h );
    std::optional<JSObject*> CreateRawBitmap();
    std::optional<JSObject*> GetColourScheme( uint32_t count );
    std::optional<pfc::string8_fast> GetColourSchemeJSON( uint32_t count );
    std::optional<JSObject*> GetGraphics();
    std::optional<std::nullptr_t> ReleaseGraphics( JsGdiGraphics* graphics );
    std::optional<JSObject*> Resize( uint32_t w, uint32_t h, uint32_t interpolationMode = 0 );
    std::optional<JSObject*> ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode );
    std::optional<std::nullptr_t> RotateFlip( uint32_t mode );
    std::optional<bool> SaveAs( const std::wstring& path, const std::wstring& format = L"image/png" );
    std::optional<bool> SaveAsWithOpt( size_t optArgCount, const std::wstring& path, const std::wstring& format );
    std::optional<std::nullptr_t> StackBlur( uint32_t radius );

public: // props
    std::optional<std::uint32_t> get_Height();
    std::optional<std::uint32_t> get_Width();

private:
    JsGdiBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap );

private:
    JSContext * pJsCtx_ = nullptr;;
    std::unique_ptr<Gdiplus::Bitmap> pGdi_;
};

}
