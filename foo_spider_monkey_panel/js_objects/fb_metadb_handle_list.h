#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsFbMetadbHandle;
class JsFbTitleFormat;

class JsFbMetadbHandleList
{
public:
    ~JsFbMetadbHandleList();
    
    static JSObject* Create( JSContext* cx, metadb_handle_list_cref handles );

    static const JSClass& GetClass();

    metadb_handle_list_cref GetHandleList() const;

public: // methods
    std::optional<std::nullptr_t> Add( JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> AddRange( JsFbMetadbHandleList* handles );
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
    std::optional<JSObject*> Item2( uint32_t index, JsFbMetadbHandle* handle );
    std::optional<JSObject*> Item2WithOpt( size_t optArgCount, uint32_t index, JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> MakeDifference( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> MakeIntersection( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> MakeUnion( JsFbMetadbHandleList* handles );
    std::optional<std::nullptr_t> OrderByFormat( JsFbTitleFormat* script, int8_t direction );
    std::optional<std::nullptr_t> OrderByPath();
    std::optional<std::nullptr_t> OrderByRelativePath();
    std::optional<std::nullptr_t> RefreshStats();
    std::optional<std::nullptr_t> Remove( JsFbMetadbHandle* handle );
    std::optional<std::nullptr_t> RemoveAll();
    std::optional<std::nullptr_t> RemoveById( uint32_t index );
    std::optional<std::nullptr_t> RemoveRange( uint32_t from, uint32_t count );
    std::optional<std::nullptr_t> Sort();
    std::optional<std::nullptr_t> UpdateFileInfoFromJSON( const pfc::string8_fast& str );

public: // props
    std::optional<uint32_t> get_Count();
    std::optional<JSObject*> get_Item( uint32_t index );
    std::optional<std::nullptr_t> put_Item( uint32_t index, JsFbMetadbHandle* handle );
    // TODO: add array methods

private:
    JsFbMetadbHandleList( JSContext* cx, metadb_handle_list_cref handles );
    JsFbMetadbHandleList( const JsFbMetadbHandleList& ) = delete;
    JsFbMetadbHandleList& operator=( const JsFbMetadbHandleList& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    metadb_handle_list metadbHandleList_;
};

}
