#pragma once

#include <js_objects/object_base.h>

#include <optional>

#include <utils/font_cache.h>

// wether to enable getters for font metrics
#define _FONT_DEV_METRICS 1

// wether to enable a getter listing the font cache
#define _FONT_DEV_CACHE 1

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;

namespace mozjs
{
    using namespace smp;

class JsGdiFont
    : public JsObjectBase<JsGdiFont>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasStaticFunctions = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

private:
    JsGdiFont( JSContext* ctx, const LOGFONTW& font );

public:
    ~JsGdiFont() override;

    static std::unique_ptr<JsGdiFont> CreateNative( JSContext* ctx, const LOGFONTW& font );
    [[nodiscard]] static size_t GetInternalSize( const LOGFONTW& );

public: // ctor
    static JSObject* Constructor
    (
        JSContext* ctx,
        const std::wstring& fontName, int32_t fontSize,  uint32_t fontStyle = 0
    );

    static JSObject* ConstructorWithOpt
    (
        JSContext* ctx, size_t optArgCount,
        const std::wstring& fontName, int32_t fontSize, uint32_t fontStyle
    );

public:
    HFONT HFont() const;

public: // props
    std::wstring get_Name() const;
    void set_Name( const std::wstring& fontName );

    uint32_t get_Height() const;
    void set_Height( uint32_t fontHeight );

    uint32_t get_Size() const;
    void set_Size( uint32_t fontSize );

    uint32_t get_Style() const;
    void set_Style( uint32_t fontStyle );

    uint32_t get_Weight() const;
    void set_Weight( uint32_t fontWeight );

    bool get_Italic() const;
    void set_Italic( bool italic );

    bool get_Underline() const;
    void set_Underline( bool underline );

    bool get_Strikeout() const;
    void set_Strikeout( bool strikeout );

#if _FONT_DEV_METRICS
    JS::Value get_Logfont() const;
    JS::Value get_Metrics() const;
    JS::Value get_DesignMetrics() const;
#endif

#if _FONT_DEV_CACHE
    JS::Value get_Cache() const;
#endif

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;

    fontcache::shared_hfont font = nullptr;

    void Reload();
    LOGFONTW logfont = {};
    TEXTMETRICW metric = {};
};

} // namespace mozjs
