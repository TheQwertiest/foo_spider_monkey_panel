#include <stdafx.h>
#include "js_engine_proxy.h"

#include <js_engine/js_engine.h>


namespace mozjs
{

void UpdateJsEngineOnHeapAllocate( uint32_t size )
{
    JsEngine::GetInstance().OnHeapAllocate( size );
}

void UpdateJsEngineOnHeapDeallocate( uint32_t size )
{
    JsEngine::GetInstance().OnHeapDeallocate( size );
}

}
