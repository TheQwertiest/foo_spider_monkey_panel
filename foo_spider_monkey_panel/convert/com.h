#include <oleauto.h>

namespace mozjs::convert::com
{

void VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval );
// assumes that variant arg is uninitialized
void JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg );

// assumes that obj is an array
void JsArrayToVariantArray( JSContext* cx, JS::HandleObject obj, int elementVariantType, VARIANT& var );

} // namespace mozjs::convert::com
