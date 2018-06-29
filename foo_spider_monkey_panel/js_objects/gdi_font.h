#pragma once

#include <js_engine/js_to_native_invoker.h>

#include <optional>


namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsGdiFontInfo;

class JsGdiFont
{
    friend class JsGdiFontInfo;
public:
    using InfoType = typename JsGdiFontInfo;

public:
    ~JsGdiFont();
    
    // static JSObject* Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged );    

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

private:
    JSContext * pJsCtx_ = nullptr;
    bool isManaged_;
    std::unique_ptr<Gdiplus::Font> pGdi_;
    HFONT hFont_ = nullptr;
};

class JsGdiFontInfo
{
public:
    using ObjectType = typename JsGdiFont;

    static const char className[];
    static uint32_t classFlags;
    static const JSFunctionSpec functions[];
    static const JSPropertySpec properties[];

    static bool ValidateCtorArguments( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged );

public:
    MJS_STATIC_WRAP_NATIVE_FN( JsGdiFont, get_Height );
    MJS_STATIC_WRAP_NATIVE_FN( JsGdiFont, get_Name );
    MJS_STATIC_WRAP_NATIVE_FN( JsGdiFont, get_Size );
    MJS_STATIC_WRAP_NATIVE_FN( JsGdiFont, get_Style );
};

}
