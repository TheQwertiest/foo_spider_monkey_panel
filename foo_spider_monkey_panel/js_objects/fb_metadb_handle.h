#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;


namespace mozjs
{

class JsFbMetadbHandle
{
public:
    ~JsFbMetadbHandle();
    
    static JSObject* Create( JSContext* cx, const metadb_handle_ptr& handle );

    static const JSClass& GetClass();

    metadb_handle_ptr& GetHandle();

public: // methods
    std::optional<std::nullptr_t> ClearStats();
    std::optional<bool> Compare( JsFbMetadbHandle* handle );
    std::optional<JSObject*> GetFileInfo();
    std::optional<std::nullptr_t> RefreshStats();
    std::optional<std::nullptr_t> SetFirstPlayed( std::string first_played );
    std::optional<std::nullptr_t> SetLastPlayed( std::string last_played );
    std::optional<std::nullptr_t> SetLoved( uint32_t loved );
    std::optional<std::nullptr_t> SetPlaycount( uint32_t playcount );
    std::optional<std::nullptr_t> SetRating( uint32_t rating );

public: // props
    std::optional<std::uint64_t> get_FileSize();
    std::optional<double> get_Length();
    std::optional<std::string> get_Path();
    std::optional<std::string> get_RawPath();
    std::optional<std::uint32_t> get_SubSong();

private:
    JsFbMetadbHandle( JSContext* cx, const metadb_handle_ptr& handle );
    JsFbMetadbHandle( const JsFbMetadbHandle& ) = delete;
    JsFbMetadbHandle& operator=( const JsFbMetadbHandle& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    metadb_handle_ptr metadbHandle_;
};

}
