#include <stdafx.h>
#include "message_blocking_scope.h"

#include <js_engine/js_engine.h>

namespace smp
{

bool MessageBlockingScope::isBlocking_ = false;

MessageBlockingScope::MessageBlockingScope()
{
    assert( !isBlocking_ );
    isBlocking_ = true;
    mozjs::JsEngine::GetInstance().OnModalWindowCreate();
}

MessageBlockingScope::~MessageBlockingScope()
{
    isBlocking_ = false;
    mozjs::JsEngine::GetInstance().OnModalWindowDestroy();
}

bool MessageBlockingScope::IsBlocking()
{
    return ( !modal_dialog_scope::can_create() || isBlocking_ );
}

} // namespace smp
