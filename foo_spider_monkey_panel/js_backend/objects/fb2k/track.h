#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;

template <>
struct JsObjectTraits<Track>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
};

class Track
    : public JsObjectBase<Track>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( Track );

public:
    ~Track() override;

    [[nodiscard]] static std::unique_ptr<Track> CreateNative( JSContext* cx, metadb_handle_ptr metadbHandle );
    [[nodiscard]] size_t GetInternalSize() const;

    metadb_handle_ptr GetHandle() const;

public:
    static JSObject* Constructor_1( JSContext* cx );
    static JSObject* Constructor_2( JSContext* cx, const qwr::u8string& path, uint32_t subTrackIndex = 0 );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, const qwr::u8string& path, uint32_t subTrackIndex );

    bool IsEqual( smp::not_null<Track*> handle );
    pfc::string8_fast FormatTitle( const qwr::u8string& query, const qwr::u8string& fallback = "<ERROR>" );
    pfc::string8_fast FormatTitleWithOpt( size_t optArgCount, const qwr::u8string& query, const qwr::u8string& fallback = "<ERROR>" );

    pfc::string8_fast get_DisplayPath() const;
    JSObject* get_InfoSnapshot() const;
    qwr::u8string get_Path() const;
    uint32_t get_SubTrackIndex() const;

private:
    [[nodiscard]] Track( JSContext* cx, metadb_handle_ptr metadbHandle );

private:
    JSContext* pJsCtx_ = nullptr;

    metadb_handle_ptr metadbHandle_;
};

} // namespace mozjs
