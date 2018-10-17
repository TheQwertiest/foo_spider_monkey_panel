#include <oleauto.h>

namespace mozjs::convert::com
{

void VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval );
// assumes that variant arg is uninitialized
void JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg );

}
