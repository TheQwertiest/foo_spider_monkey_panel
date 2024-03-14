#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class FsPromises;

template <>
struct JsObjectTraits<FsPromises>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
};

class FsPromises
    : public JsObjectBase<FsPromises>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( FsPromises );

    using FsTaskFn = std::function<std::function<JS::Value()>()>;

public:
    ~FsPromises() override;

    [[nodiscard]] static std::unique_ptr<FsPromises> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

    // TODO: add constants

public:
    JSObject* ReadDir( JS::HandleValue path, JS::HandleValue options = JS::UndefinedHandleValue ) const;
    JSObject* ReadDirWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const;
    JSObject* ReadFile( JS::HandleValue path, JS::HandleValue options = JS::UndefinedHandleValue ) const;
    JSObject* ReadFileWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const;
    JSObject* Stat( JS::HandleValue path, JS::HandleValue options = JS::UndefinedHandleValue ) const;
    JSObject* StatWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const;
    JSObject* WriteFile( JS::HandleValue path, JS::HandleValue data, JS::HandleValue options = JS::UndefinedHandleValue ) const;
    JSObject* WriteFileWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue data, JS::HandleValue options ) const;

private:
    [[nodiscard]] FsPromises( JSContext* cx );

    void EnqueueFsTask( FsTaskFn taskFn, JS::HandleObject jsTarget ) const;

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs
