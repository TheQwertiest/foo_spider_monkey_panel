#include <stdafx.h>

#include "message_blocking_scope.h"

#include <js_engine/js_engine.h>

namespace smp
{

std::atomic_bool MessageBlockingScope::isBlocking_ = false;

MessageBlockingScope::MessageBlockingScope()
{
    assert( !isBlocking_ );
    isBlocking_ = true;
}

MessageBlockingScope::~MessageBlockingScope()
{
    isBlocking_ = false;
}

bool MessageBlockingScope::IsBlocking()
{
    return ( !modal_dialog_scope::can_create() || isBlocking_ );
}

} // namespace smp
