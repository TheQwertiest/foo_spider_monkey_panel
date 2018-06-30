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
    std::optional<uint32_t> get_Height() const;
    std::optional<std::wstring> get_Name() const;
    std::optional<float> get_Size() const;
    std::optional<uint32_t> get_Style() const;

private:
    JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged );
    JsGdiFont( const JsGdiFont& ) = delete;
    JsGdiFont& operator=( const JsGdiFont& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    bool isManaged_;
    std::unique_ptr<Gdiplus::Font> pGdi_;
    HFONT hFont_ = nullptr;
};

}
