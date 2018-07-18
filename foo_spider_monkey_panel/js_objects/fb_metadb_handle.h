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
    ~JsFbMetadbHandle();

    static std::unique_ptr<JsFbMetadbHandle> CreateNative( JSContext* cx, const metadb_handle_ptr& handle );    
    static size_t GetInternalSize( const metadb_handle_ptr& handle );
    
public:
    metadb_handle_ptr& GetHandle();

public: // methods
    std::optional<std::nullptr_t> ClearStats();
    std::optional<bool> Compare( JsFbMetadbHandle* handle );
    std::optional<JSObject*> GetFileInfo();
    std::optional<std::nullptr_t> RefreshStats();
    std::optional<std::nullptr_t> SetFirstPlayed( const pfc::string8_fast& first_played );
    std::optional<std::nullptr_t> SetLastPlayed( const pfc::string8_fast& last_played );
    std::optional<std::nullptr_t> SetLoved( uint32_t loved );
    std::optional<std::nullptr_t> SetPlaycount( uint32_t playcount );
    std::optional<std::nullptr_t> SetRating( uint32_t rating );

public: // props
    std::optional<std::uint64_t> get_FileSize();
    std::optional<double> get_Length();
    std::optional<pfc::string8_fast> get_Path();
    std::optional<pfc::string8_fast> get_RawPath();
    std::optional<std::uint32_t> get_SubSong();

private:
    JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle );    

private:
    JSContext * pJsCtx_ = nullptr;
    metadb_handle_ptr metadbHandle_;
};

}
