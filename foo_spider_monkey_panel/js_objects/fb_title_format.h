#pragma once

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
{
public:
    ~JsFbTitleFormat();

    static JSObject* Create( JSContext* cx, const pfc::string8_fast& expr );

    static const JSClass& GetClass();

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
