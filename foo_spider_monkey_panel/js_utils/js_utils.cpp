#include <stdafx.h>
#include "js_utils.h"


namespace mozjs
{

JSObject* GetJsObjectFromValue( JSContext* cx, JS::HandleValue jsValue )
{
    if ( !jsValue.isObject() )
    {        
        return false;
    }

    return jsValue.toObjectOrNull();
}

}
