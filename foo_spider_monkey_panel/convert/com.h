#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

namespace mozjs::convert::com
{

bool VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval );
bool JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg );

}
