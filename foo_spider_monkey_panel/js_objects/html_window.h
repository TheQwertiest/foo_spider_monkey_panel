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
    ~JsHtmlWindow() = default;

    static std::unique_ptr<JsHtmlWindow> CreateNative( JSContext* cx, const std::wstring& htmlCode, JS::HandleValue options );
    static size_t GetInternalSize( const std::wstring& htmlCode, JS::HandleValue options );

public: 
    std::optional<nullptr_t> Close();
    std::optional<nullptr_t> Focus();

public:
    std::optional<bool> get_IsClosed();

private:    
    // alias for MSHTML::IHTMLWindow2Ptr: don't want to drag #import into headers
    using HtmlWindow2ComPtr = _com_ptr_t<_com_IIID<IHTMLWindow2, &__uuidof( IHTMLWindow2 )>>;

    JsHtmlWindow( JSContext* cx, DWORD pid, HtmlWindow2ComPtr pWindow );

private:
    JSContext * pJsCtx_ = nullptr;
    DWORD pid_ = 0;
    HtmlWindow2ComPtr pWindow_;
};

}
