#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
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
    JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged );
    JsGdiFont( const JsGdiFont& ) = delete;

private:
    JSContext * pJsCtx_;
    bool isManaged_;
    std::unique_ptr<Gdiplus::Font> pGdi_;
    HFONT hFont_;
};

}
