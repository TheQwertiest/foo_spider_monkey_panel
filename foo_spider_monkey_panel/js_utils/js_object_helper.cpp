#include <stdafx.h>

#include "js_object_helper.h"

namespace mozjs
{

bool DummyGetter( JSContext*, unsigned, JS::Value* vp )
{
    vp->setUndefined();
    return true;
}

const void* GetSmpProxyFamily()
{
    // family must contain unique pointer, so the value does not really matter
    static const char kProxyFamilyVar = 'Q';
    return &kProxyFamilyVar;
}

} // namespace mozjs
