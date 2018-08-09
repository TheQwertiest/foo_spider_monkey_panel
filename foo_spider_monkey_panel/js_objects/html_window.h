#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <memory>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;
struct JSFunctionSpec;
struct JSPropertySpec;


namespace mozjs
{

class JsHtmlWindow
    : public JsObjectBase<JsHtmlWindow>
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
    ~JsHtmlWindow();

    static std::unique_ptr<JsHtmlWindow> CreateNative( JSContext* cx, const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback );
    static size_t GetInternalSize( const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback );

public: 
    std::optional<nullptr_t> Close();

public: 


private:
    // alias for MSHTML::IHTMLWindow2Ptr: don't want to drag #import into headers
    using HtmlWindow2ComPtr = _com_ptr_t<_com_IIID<IHTMLDocument2, &__uuidof(IHTMLDocument2)> >;

    JsHtmlWindow( JSContext* cx, HtmlWindow2ComPtr pHtaWindow );

private:
    JSContext * pJsCtx_ = nullptr;
    HtmlWindow2ComPtr pHtaWindow_;
};

}
