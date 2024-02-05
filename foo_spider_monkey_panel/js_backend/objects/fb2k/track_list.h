#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class Track;
class TrackList;

template <>
struct JsObjectTraits<TrackList>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSFunctionSpec* JsStaticFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
    static const js::BaseProxyHandler& JsProxy;
};

class TrackList final
    : public JsObjectBase<TrackList>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackList );

public:
    ~TrackList() override = default;

    static std::unique_ptr<TrackList> CreateNative( JSContext* cx, const metadb_handle_list& tracks );
    [[nodiscard]] size_t GetInternalSize() const;

    [[nodiscard]] const metadb_handle_list& GetHandleList() const;
    [[nodiscard]] metadb_handle_list& GetMutableHandleList();

    JS::Value GetItem( uint32_t index ) const;
    void PutItem( uint32_t index, smp::not_null<Track*> track );

    static metadb_handle_list ValueToHandleList( JSContext* cx, JS::HandleValue tracks );

public:
    static JSObject* Constructor( JSContext* cx, JS::HandleValue value = JS::UndefinedHandleValue );
    static JSObject* ConstructorWithOpt( JSContext* cx, size_t optArgCount, JS::HandleValue value );

    // TODO: add filter by query

    void Clear();
    JSObject* Concat( JS::HandleValue tracks ) const;
    void ConcatInPlace( JS::HandleValue tracks );
    JSObject* Difference( smp::not_null<TrackList*> tracks ) const;
    double GetTotalDuration() const;
    uint64_t GetTotalSize() const;
    int32_t IndexOf( smp::not_null<Track*> track ) const;
    JSObject* Intersection( smp::not_null<TrackList*> tracks ) const;
    void OptimizeForValueSearch( bool removeDuplicates = false );
    void OptimizeForValueSearchWithOpt( size_t optArgCount, bool removeDuplicates );
    // TODO: return removed values
    // TODO: add array support
    void PullAt( int32_t index );
    void Pull( smp::not_null<Track*> track );
    void Push( smp::not_null<Track*> track );
    JSObject* SaveToPlaylistFile( const qwr::u8string& path );
    void SortByFormat( const qwr::u8string& query, int8_t direction = 1 );
    void SortByFormatWithOpt( size_t optArgCount, const qwr::u8string& query, int8_t direction );
    void SortByPath();
    // TODO: move to library
    void SortByRelativePath();
    JSObject* Splice( int32_t start, JS::HandleValue deleteCount = JS::UndefinedHandleValue, JS::HandleValue tracks = JS::UndefinedHandleValue );
    JSObject* SpliceWithOpt( size_t optArgCount, int32_t start, JS::HandleValue deleteCount, JS::HandleValue tracks );
    JS::Value ToArray() const;
    JSObject* Union( smp::not_null<TrackList*> tracks ) const;

    JSObject* CreateIterator( JS::HandleObject jsSelf ) const;

    static JSObject* CreateFromPaths( JSContext* cx, const std::vector<qwr::u8string>& paths );

    uint32_t get_Length() const;

private:
    TrackList( JSContext* cx, const metadb_handle_list& tracks );

private:
    JSContext* pJsCtx_ = nullptr;
    metadb_handle_list metadbHandleList_;
    bool isSorted_ = false;
};

} // namespace mozjs
