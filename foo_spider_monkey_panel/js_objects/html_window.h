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

public: 


private:
    JsHtmlWindow( JSContext* cx );

private:
    JSContext * pJsCtx_ = nullptr;
};

}
