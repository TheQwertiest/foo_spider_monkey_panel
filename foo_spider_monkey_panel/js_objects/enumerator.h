#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;

namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsEnumerator
    : public JsObjectBase<JsEnumerator>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsEnumerator() = default;

    static std::unique_ptr<JsEnumerator> CreateNative( JSContext* cx, IUnknown* pUnknown );
    static size_t GetInternalSize( IUnknown* pUnknown );

public:
    bool AtEnd();
    JS::Value Item();
    void MoveFirst();
    void MoveNext();

private:
    // alias for IEnumVARIANTPtr: don't want to drag extra com headers
    using EnumVARIANTComPtr = _com_ptr_t<_com_IIID<IEnumVARIANT, &__uuidof( IEnumVARIANT )>>;

    JsEnumerator( JSContext* cx, EnumVARIANTComPtr pEnum, bool hasElements );

    void GetCurrentElement();

private:
    JSContext* pJsCtx_ = nullptr;
    EnumVARIANTComPtr pEnum_ = nullptr;
    _variant_t curElem_;
    bool hasElements_ = false;
    bool isAtEnd_ = false;
};

} // namespace mozjs
