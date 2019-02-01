#pragma once

#include <js_objects/object_base.h>

#pragma warning( push )
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <js/Proxy.h>
#pragma warning( pop )

#include <nlohmann/json.hpp>

#include <optional>


class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Font;
}

namespace js
{
class BaseProxyHandler;
}

namespace mozjs
{

class JsFbMetadbHandle;
class JsFbTitleFormat;

class JsFbMetadbHandleList
    : public JsObjectBase<JsFbMetadbHandleList>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = true;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
    static const js::BaseProxyHandler& JsProxy;

public:
    ~JsFbMetadbHandleList();

    static std::unique_ptr<JsFbMetadbHandleList> CreateNative( JSContext* cx, const metadb_handle_list& handles );
    static size_t GetInternalSize( const metadb_handle_list& handles );

public:
    const metadb_handle_list& GetHandleList() const;

public: // ctor
    static JSObject* Constructor( JSContext* cx, JS::HandleValue jsValue = JS::UndefinedHandleValue );

public: // methods
    void Add( JsFbMetadbHandle* handle );
    void AddRange( JsFbMetadbHandleList* handles );
    void AttachImage( const pfc::string8_fast& image_path, uint32_t art_id );
    int32_t BSearch( JsFbMetadbHandle* handle );
    double CalcTotalDuration();
    std::uint64_t CalcTotalSize();
    JSObject* Clone();
    // TODO: rename to ToArray()
    JSObject* Convert();
    int32_t Find( JsFbMetadbHandle* handle );
    JSObject* GetLibraryRelativePaths();
    void Insert( uint32_t index, JsFbMetadbHandle* handle );
    void InsertRange( uint32_t index, JsFbMetadbHandleList* handles );
    void MakeDifference( JsFbMetadbHandleList* handles );
    void MakeIntersection( JsFbMetadbHandleList* handles );
    void MakeUnion( JsFbMetadbHandleList* handles );
    void OrderByFormat( JsFbTitleFormat* script, int8_t direction );
    void OrderByPath();
    void OrderByRelativePath();
    void RefreshStats();
    void Remove( JsFbMetadbHandle* handle );
    void RemoveAll();
    void RemoveAttachedImage( uint32_t art_id );
    void RemoveAttachedImages();
    void RemoveById( uint32_t index );
    void RemoveRange( uint32_t from, uint32_t count );
    void Sort();
    void UpdateFileInfoFromJSON( const pfc::string8_fast& str );

public: // props
    uint32_t get_Count();
    JSObject* get_Item( uint32_t index );
    void put_Item( uint32_t index, JsFbMetadbHandle* handle );

private:
    static void ModifyFileInfoWithJson( const nlohmann::json& jsonObject, file_info_impl& fileInfo );

private:
    JsFbMetadbHandleList( JSContext* cx, const metadb_handle_list& handles );

private:
    JSContext* pJsCtx_ = nullptr;
    metadb_handle_list metadbHandleList_;
};

} // namespace mozjs
