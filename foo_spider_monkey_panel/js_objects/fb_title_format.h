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

    static JSObject* Create( JSContext* cx, const std::string& expr );

    static const JSClass& GetClass();

public:
    std::optional<std::string> Eval( bool force );
    std::optional<std::string> EvalWithMetadb( JsFbMetadbHandle* handle );
    std::optional<std::string> EvalWithMetadbs( JsFbMetadbHandleList* handles );

private:
    JsFbTitleFormat( JSContext* cx, const std::string& expr );
    JsFbTitleFormat( const JsFbTitleFormat& ) = delete;
    JsFbTitleFormat& operator=( const JsFbTitleFormat& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    titleformat_object::ptr titleFormatObject_;
};

}
