#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

#include <filesystem>

namespace mozjs
{

class FsStats;

template <>
struct JsObjectTraits<FsStats>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class FsStats
    : public JsObjectBase<FsStats>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( FsStats );

public:
    ~FsStats() override;

    // TODO: add bigint support
    [[nodiscard]] static std::unique_ptr<FsStats> CreateNative( JSContext* cx, const std::filesystem::path& path );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    bool IsDirectory() const;
    bool IsFile() const;

public:
    JS::Value get_Size() const;

private:
    [[nodiscard]] FsStats( JSContext* cx, const std::filesystem::path& path );

private:
    JSContext* pJsCtx_ = nullptr;

    const std::filesystem::path path_;
};

} // namespace mozjs
