#pragma once

#include <js_objects/object_base.h>
#include <js_utils/js_heap_helper.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;

namespace mozjs
{

class JsFbMetadbHandleList;

class JsFbMetadbHandleList_Iterator
    : public JsObjectBase<JsFbMetadbHandleList_Iterator>
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
    ~JsFbMetadbHandleList_Iterator() override;

    static std::unique_ptr<JsFbMetadbHandleList_Iterator> CreateNative( JSContext* cx, JsFbMetadbHandleList& handleList );
    static size_t GetInternalSize( JsFbMetadbHandleList& handleList );

public:
    JSObject* Next();

private:
    JsFbMetadbHandleList_Iterator( JSContext* cx, JsFbMetadbHandleList& handleList );

private:
    JSContext* pJsCtx_ = nullptr;
    JsFbMetadbHandleList& handleList_;

    HeapHelper heapHelper_;
    std::optional<uint32_t> jsNextId_;

    uint32_t curPosition_ = 0;
};

} // namespace mozjs
