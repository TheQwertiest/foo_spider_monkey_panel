#include <stdafx.h>

#include "cancellable.h"

namespace smp
{

bool Cancellable::IsCancelled() const
{
    assert( core_api::is_main_thread() );
    return isCanceled_;
}

void Cancellable::Cancel()
{
    assert( core_api::is_main_thread() );
    isCanceled_ = true;
}

} // namespace smp
