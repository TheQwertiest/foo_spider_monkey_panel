#pragma once

#include <js_objects/object_base.h>

#include <optional>
#include <string>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsFbMetadbHandle;
class JsFbMetadbHandleList;

class JsFbTitleFormat
    : public JsObjectBase<JsFbTitleFormat>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasStaticFunctions = false;
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsFbTitleFormat() override = default;

    static std::unique_ptr<JsFbTitleFormat> CreateNative( JSContext* cx, const qwr::u8string& expr );
    static size_t GetInternalSize( const qwr::u8string& expr );

public:
    titleformat_object::ptr GetTitleFormat();

public: // ctor
    static JSObject* Constructor( JSContext* cx, const qwr::u8string& expr );

public:
    pfc::string8_fast Eval( bool force = false );
    pfc::string8_fast EvalWithOpt( size_t optArgCount, bool force );
    pfc::string8_fast EvalWithMetadb( JsFbMetadbHandle* handle );
    JS::Value EvalWithMetadbs( JsFbMetadbHandleList* handles );

private:
    JsFbTitleFormat( JSContext* cx, const qwr::u8string& expr );

private:
    JSContext* pJsCtx_ = nullptr;
    titleformat_object::ptr titleFormatObject_;
};

} // namespace mozjs
