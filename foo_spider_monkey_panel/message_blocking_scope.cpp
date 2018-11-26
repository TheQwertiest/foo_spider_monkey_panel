#include <stdafx.h>
#include "message_blocking_scope.h"

namespace smp
{

bool MessageBlockingScope::isBlocking_ = false;

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
