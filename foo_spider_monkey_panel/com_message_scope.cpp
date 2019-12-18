#include <stdafx.h>

#include "com_message_scope.h"

namespace smp
{

uint32_t ComMessageScope::scopeRefs_ = 0;

ComMessageScope::ComMessageScope()
{
    ++scopeRefs_;
}

ComMessageScope::~ComMessageScope()
{
    assert( scopeRefs_ );
    --scopeRefs_;
}

bool ComMessageScope::IsInScope()
{
    return !!scopeRefs_;
}

} // namespace smp
