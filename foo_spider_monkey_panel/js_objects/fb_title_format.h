#pragma once

#include <js_objects/object_base.h>

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
    static constexpr bool HasProxy = false;
    static constexpr bool HasPostCreate = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;

public:
    ~JsFbTitleFormat();

    static std::unique_ptr<JsFbTitleFormat> CreateNative( JSContext* cx, const pfc::string8_fast& expr );
    static size_t GetInternalSize( const pfc::string8_fast& expr );

public:
    titleformat_object::ptr GetTitleFormat();

public: // ctor
    static JSObject* Constructor( JSContext* cx, const pfc::string8_fast& expr );

public:
    pfc::string8_fast Eval( bool force = false );
    pfc::string8_fast EvalWithOpt( size_t optArgCount, bool force );
    pfc::string8_fast EvalWithMetadb( JsFbMetadbHandle* handle );
    JSObject* EvalWithMetadbs( JsFbMetadbHandleList* handles );

private:
    JsFbTitleFormat( JSContext* cx, const pfc::string8_fast& expr );

private:
    JSContext* pJsCtx_ = nullptr;
    titleformat_object::ptr titleFormatObject_;
};

} // namespace mozjs
