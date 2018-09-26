#include <oleauto.h>

namespace mozjs::convert::com
{

bool VariantToJs( JSContext* cx, VARIANTARG& var, JS::MutableHandleValue rval );
// assumes that variant arg is uninitialized
bool JsToVariant( JSContext* cx, JS::HandleValue rval, VARIANTARG& arg );

}
