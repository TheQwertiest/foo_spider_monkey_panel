#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class TrackInfoSnapshot;

template <>
struct JsObjectTraits<TrackInfoSnapshot>
{
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
};

class TrackInfoSnapshot final
    : public JsObjectBase<TrackInfoSnapshot>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( TrackInfoSnapshot );

public:
    ~TrackInfoSnapshot() override = default;

    static std::unique_ptr<TrackInfoSnapshot> CreateNative( JSContext* cx, metadb_info_container::ptr infoContainer );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    uint32_t GetMetaCount() const;
    int32_t GetMetaIndex( const qwr::u8string& name ) const;
    qwr::u8string GetMetaName( uint32_t index ) const;
    qwr::u8string GetMetaValue( uint32_t index, uint32_t valueIndex ) const;
    uint32_t GetMetaValueCount( uint32_t index ) const;
    uint32_t GetTechnicalInfoCount() const;
    int32_t GetTechnicalInfoIndex( const qwr::u8string& name ) const;
    qwr::u8string GetTechnicalInfoName( uint32_t index ) const;
    qwr::u8string GetTechnicalInfoValue( uint32_t index ) const;

    uint64_t get_FileSize() const;
    double get_Length() const;

private:
    TrackInfoSnapshot( JSContext* cx, metadb_info_container::ptr infoContainer );

private:
    [[maybe_unused]] JSContext* pJsCtx_ = nullptr;
    metadb_info_container::ptr infoContainer_;
    const file_info& fileInfo_;
    const t_filestats& fileStats_;
};

} // namespace mozjs
