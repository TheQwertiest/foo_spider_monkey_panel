#pragma once

#include <js_engine/js_error_codes.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdi
{
class Font;
}

namespace mozjs
{

class JsGdiFont
{
public:
    ~JsGdiFont();
    
    static JSObject* Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged );

    static const JSClass& GetClass();

public: 
    Gdiplus::Font* GdiFont() const;
    HFONT HFont() const;

public: // props
    std::optional<uint32_t> Height() const;
    std::optional<std::wstring> Name() const;
    std::optional<float> Size() const;
    std::optional<uint32_t> Style() const;

private:
    JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont );
    JsGdiFont( const JsGdiFont& ) = delete;

private:
    JSContext * pJsCtx_;
    std::unique_ptr<Gdiplus::Font> gdiFont_;
    HFONT hFont_;
};

}
