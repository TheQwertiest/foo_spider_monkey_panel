#pragma once

#include <js_objects/object_base.h>

#include <optional>

#include <wil/resource.h>

// purge unused elements from the cache every N-th access
#define FONT_CACHE_PURGE_FREQ 16

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

    wil::shared_hfont font = nullptr;

    void ReloadMetrics();
    LOGFONTW logfont = {};
    TEXTMETRICW metric = {};
};

} // namespace mozjs

namespace
{
// wether to use a custom hash combiner (with supposedly less collisions), or one based on boost's
#define HASH_CUSTOM_COMBINER 1
// wether to combine size + weight + italic + underlune + striketrough into one hash or not
// or: hash_combine once, or once for each property
#define HASH_PACKED_PROPS    1

#if HASH_CUSTOM_COMBINER
inline const size_t mxs( const size_t m, const size_t& x ) noexcept
{
    return m * ( x ^ ( x >> 16 ) );
}

inline const size_t dsp( const size_t& z ) noexcept
{
    constexpr size_t a = 0x55555555; // alternating 0101
    constexpr size_t r = 0x2e5bf271; // random uneven int constant
    return mxs( r, mxs( a, z ) );
}

inline const size_t cohash( const size_t& seed, const size_t& hash ) noexcept
{
    return std::rotl( seed, 11 ) ^ dsp( hash );
}
#else
inline const size_t cohash( const size_t& seed, const size_t& add ) noexcept
{
    constexpr size_t boost_magic = 0x9e3779b9;
    return ( seed ^ add ) + boost_magic + ( seed << 6 ) + ( seed >> 2 );
}
#endif

inline const size_t hash_combine( const std::size_t& seed ) noexcept
{
    return seed;
}

template <typename T, typename... Rest>
inline const size_t hash_combine( const size_t& seed, const T& val, Rest... rest ) noexcept
{
    return hash_combine( cohash( seed, std::hash<T>{}( val ) ), rest... );
};

} // namespace

namespace std
{
template <>
struct hash<LOGFONTW>
{
    inline const size_t operator()( const LOGFONTW& logfont ) const noexcept
    {
        return hash_combine
        (
            hash<wstring>{}( wstring( logfont.lfFaceName ) ),
#if HASH_PACKED_PROPS
            ( ( logfont.lfHeight            ) << 13 ) |
            ( ( logfont.lfWeight    & 0x3ff ) <<  3 ) |
            ( ( logfont.lfItalic    ? 1 : 0 ) <<  2 ) |
            ( ( logfont.lfUnderline ? 1 : 0 ) <<  1 ) |
            ( ( logfont.lfStrikeOut ? 1 : 0 )       )
#else
            logfont.lfHeight,
            logfont.lfWeight,
            logfont.lfItalic,
            logfont.lfUnderline,
            logfont.lfStrikeOut
#endif
        );
    }
};

template<>
struct equal_to<LOGFONTW>
{
    inline bool operator()( const LOGFONTW& lhs, const LOGFONTW& rhs ) const noexcept
    {
        return hash<LOGFONTW>{}( lhs ) == hash<LOGFONTW>{}( rhs );
    }
};

} // namespace std
