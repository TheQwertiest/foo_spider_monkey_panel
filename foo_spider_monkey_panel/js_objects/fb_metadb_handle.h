#pragma once

#include <js_objects/object_base.h>

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle
    : public JsObjectBase<JsFbMetadbHandle>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;

public:
    ~JsFbMetadbHandle() override = default;

    static std::unique_ptr<JsFbMetadbHandle> CreateNative( JSContext* cx, const metadb_handle_ptr& handle );
    static size_t GetInternalSize( const metadb_handle_ptr& handle );

public:
    metadb_handle_ptr& GetHandle();

public: // methods
    void ClearStats();
    bool Compare( JsFbMetadbHandle* handle );
    JSObject* GetFileInfo();
    void RefreshStats();
    void SetFirstPlayed( const qwr::u8string& first_played );
    void SetLastPlayed( const qwr::u8string& last_played );
    void SetLoved( uint32_t loved );
    void SetPlaycount( uint32_t playcount );
    void SetRating( uint32_t rating );

public: // props
    int64_t get_FileSize();
    double get_Length();
    qwr::u8string get_Path();
    qwr::u8string get_RawPath();
    uint32_t get_SubSong();

private:
    JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle );

private:
    JSContext* pJsCtx_ = nullptr;
    metadb_handle_ptr metadbHandle_;
};

} // namespace mozjs
