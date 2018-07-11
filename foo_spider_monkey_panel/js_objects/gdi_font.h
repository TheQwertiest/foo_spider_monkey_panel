#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;

namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsGdiFont
    : public JsObjectBase<JsGdiFont>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsGdiFont();

    static std::unique_ptr<JsGdiFont> CreateNative( JSContext* cx, std::unique_ptr<Gdiplus::Font> pGdiFont, HFONT hFont, bool isManaged );

public: 
    Gdiplus::Font* GdiFont() const;
    HFONT GetHFont() const;

public: // props
    std::optional<uint32_t> get_Height() const;
    std::optional<std::wstring> get_Name() const;
    std::optional<float> get_Size() const;
    std::optional<uint32_t> get_Style() const;

private:
    JsGdiFont( JSContext* cx, std::unique_ptr<Gdiplus::Font> gdiFont, HFONT hFont, bool isManaged );    

private:
    JSContext * pJsCtx_ = nullptr;
    bool isManaged_;
    std::unique_ptr<Gdiplus::Font> pGdi_;
    HFONT hFont_ = nullptr;
};

}
