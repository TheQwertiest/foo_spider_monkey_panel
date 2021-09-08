#pragma once

#include <js_objects/object_base.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Proxy.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

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
    static constexpr bool HasStaticFunctions = false;
    static constexpr bool HasProxy = true;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
    static const js::BaseProxyHandler& JsProxy;

public:
    ~JsFbMetadbHandleList() override = default;

    static std::unique_ptr<JsFbMetadbHandleList> CreateNative( JSContext* cx, const metadb_handle_list& handles );
    [[nodiscard]] static size_t GetInternalSize( const metadb_handle_list& handles );

public:
    [[nodiscard]] const metadb_handle_list& GetHandleList() const;

public: // ctor
    static JSObject* Constructor( JSContext* cx, JS::HandleValue jsValue = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, JS::HandleValue jsValue );

public: // methods
    void Add( JsFbMetadbHandle* handle );
    void AddRange( JsFbMetadbHandleList* handles );
    void AttachImage( const qwr::u8string& image_path, uint32_t art_id );
    int32_t BSearch( JsFbMetadbHandle* handle );
    double CalcTotalDuration();
    std::uint64_t CalcTotalSize();
    JSObject* Clone();
    // TODO v2: rename to ToArray()
    JS::Value Convert();
    int32_t Find( JsFbMetadbHandle* handle );
    JS::Value GetLibraryRelativePaths();
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
    void UpdateFileInfoFromJSON( const qwr::u8string& str );

    JSObject* CreateIterator();

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
