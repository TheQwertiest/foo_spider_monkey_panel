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
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsGdiBitmap();

    static std::unique_ptr<JsGdiBitmap> CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap );
    static size_t GetInternalSize( const std::unique_ptr<Gdiplus::Bitmap>& gdiBitmap );

public:
    Gdiplus::Bitmap* GdiBitmap() const;

public: // ctor
    static JSObject* Constructor( JSContext* cx, JsGdiBitmap* other );

public: //methods
    JSObject* ApplyAlpha( uint8_t alpha );
    bool ApplyMask( JsGdiBitmap* mask );
    JSObject* Clone( float x, float y, float w, float h );
    JSObject* CreateRawBitmap();
    JSObject* GetColourScheme( uint32_t count );
    pfc::string8_fast GetColourSchemeJSON( uint32_t count );
    JSObject* GetGraphics();
    void ReleaseGraphics( JsGdiGraphics* graphics );
    JSObject* Resize( uint32_t w, uint32_t h, uint32_t interpolationMode = 0 );
    JSObject* ResizeWithOpt( size_t optArgCount, uint32_t w, uint32_t h, uint32_t interpolationMode );
    void RotateFlip( uint32_t mode );
    bool SaveAs( const std::wstring& path, const std::wstring& format = L"image/png" );
    bool SaveAsWithOpt( size_t optArgCount, const std::wstring& path, const std::wstring& format );
    void StackBlur( uint32_t radius );

public: // props
    std::uint32_t get_Height();
    std::uint32_t get_Width();

private:
    JsGdiBitmap( JSContext* cx, std::unique_ptr<Gdiplus::Bitmap> gdiBitmap );

private:
    JSContext* pJsCtx_ = nullptr;
    ;
    std::unique_ptr<Gdiplus::Bitmap> pGdi_;
};

} // namespace mozjs
