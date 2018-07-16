#pragma once

#include <stdint.h>

namespace mozjs
{

void UpdateJsEngineOnHeapAllocate( uint32_t size );
void UpdateJsEngineOnHeapDeallocate( uint32_t size );

}
