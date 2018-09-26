#include <stdafx.h>
#include "js_object_helper.h"


namespace mozjs
{

bool DummyGetter( JSContext*, unsigned, JS::Value* vp )
{
    vp->setUndefined();
    return true;
}

}
