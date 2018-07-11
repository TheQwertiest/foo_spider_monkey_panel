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
    static constexpr bool HasProxy = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;

public:
    ~JsFbTitleFormat();

    static std::unique_ptr<JsFbTitleFormat> CreateNative( JSContext* cx, const pfc::string8_fast& expr );

public:
    titleformat_object::ptr GetTitleFormat();

public:
    std::optional<pfc::string8_fast> Eval( bool force = false );
    std::optional<pfc::string8_fast> EvalWithOpt( size_t optArgCount, bool force );
    std::optional<pfc::string8_fast> EvalWithMetadb( JsFbMetadbHandle* handle );
    std::optional<JSObject*> EvalWithMetadbs( JsFbMetadbHandleList* handles );

private:
    JsFbTitleFormat( JSContext* cx, const pfc::string8_fast& expr );
    JsFbTitleFormat( const JsFbTitleFormat& ) = delete;
    JsFbTitleFormat& operator=( const JsFbTitleFormat& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    titleformat_object::ptr titleFormatObject_;
};

}
