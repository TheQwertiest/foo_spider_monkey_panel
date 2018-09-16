#pragma once

#include <js_objects/object_base.h>

#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <js/Proxy.h>
#pragma warning( pop ) 

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
    // TODO: add global proto
    static constexpr bool HasGlobalProto = false;
    static constexpr bool HasProxy = true;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const js::BaseProxyHandler& JsProxy;

public:
    ~JsFbMetadbHandleList();

    static std::unique_ptr<JsFbMetadbHandleList> CreateNative( JSContext* cx, metadb_handle_list_cref handles );
    static size_t GetInternalSize( metadb_handle_list_cref handles );

public:
    metadb_handle_list_cref GetHandleList() const;

public: // methods
    std::optional<std::nullptr_t> Add( JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> AddRange( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> AttachImage( const pfc::string8_fast& image_path, uint32_t art_id );
    std::optional<int32_t> BSearch( JsFbMetadbHandle* handle );
    std::optional<double> CalcTotalDuration();
    std::optional<std::uint64_t> CalcTotalSize();
    std::optional<JSObject*> Clone();
    // TODO: rename to ToArray()
    std::optional<JSObject*> Convert();
    std::optional<int32_t> Find( JsFbMetadbHandle* handle );
    std::optional<JSObject*> GetLibraryRelativePaths();
    std::optional<std::nullptr_t> Insert( uint32_t index, JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> InsertRange( uint32_t index, JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> MakeDifference( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> MakeIntersection( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> MakeUnion( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> OrderByFormat( JsFbTitleFormat* script, int8_t direction );
    std::optional<std::nullptr_t> OrderByPath();
    std::optional<std::nullptr_t> OrderByRelativePath();
    std::optional<std::nullptr_t> RefreshStats();
    std::optional<std::nullptr_t> Remove( JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> RemoveAll();
    std::optional<std::nullptr_t> RemoveAttachedImage( uint32_t art_id );
    std::optional<std::nullptr_t> RemoveById( uint32_t index );
    std::optional<std::nullptr_t> RemoveRange( uint32_t from, uint32_t count );
    std::optional<std::nullptr_t> Sort();
    std::optional<std::nullptr_t> UpdateFileInfoFromJSON( const pfc::string8_fast& str );

public: // props
    std::optional<uint32_t> get_Count();

public:
    std::optional<JSObject*> get_Item( uint32_t index );
    std::optional<std::nullptr_t> put_Item( uint32_t index, JsFbMetadbHandle* handle );

private:
    JsFbMetadbHandleList( JSContext* cx, metadb_handle_list_cref handles );

private:
    JSContext * pJsCtx_ = nullptr;
    metadb_handle_list metadbHandleList_;
};

}
