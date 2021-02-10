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

class JsActiveXObject;

class JsActiveXObject_Iterator
    : public JsObjectBase<JsActiveXObject_Iterator>
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
    ~JsActiveXObject_Iterator() override;

    static std::unique_ptr<JsActiveXObject_Iterator> CreateNative( JSContext* cx, JsActiveXObject& activeXObject );
    static size_t GetInternalSize( JsActiveXObject& activeXObject );

public:
    JSObject* Next();

public:
    static bool IsIterable( const JsActiveXObject& activeXObject );

private:
    // alias for IEnumVARIANTPtr: don't want to drag extra com headers
    using EnumVARIANTComPtr = _com_ptr_t<_com_IIID<IEnumVARIANT, &__uuidof( IEnumVARIANT )>>;

    JsActiveXObject_Iterator( JSContext* cx, EnumVARIANTComPtr pEnum );

    void LoadCurrentElement();

private:
    JSContext* pJsCtx_ = nullptr;
    EnumVARIANTComPtr pEnum_ = nullptr;

    HeapHelper heapHelper_;
    std::optional<uint32_t> jsNextId_;

    _variant_t curElem_;
    bool isAtEnd_ = true;
};

} // namespace mozjs
