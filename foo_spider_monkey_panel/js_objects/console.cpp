#include <stdafx.h>

#include "console.h"

#include <js/Conversions.h>

namespace
{

bool Log( JSContext* cx, unsigned argc, JS::Value* vp )
{
     JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

     std::string outputString;

     for ( unsigned i = 0; i < args.length(); i++ )
     {
          JS::RootedString str( cx, JS::ToString( cx, args[i] ) );
          if ( !str )
          {
               return false;
          }

          char* bytes = JS_EncodeStringToUTF8( cx, str );
          if ( !bytes )
          {
               return false;
          }

          outputString += bytes;
          if ( i < args.length() )
          {
              outputString += ' ';
          }

          JS_free( cx, bytes );
     }

     args.rval().setUndefined();

     console::printf( outputString.c_str() );
     return true;
}

static const JSFunctionSpec console_functions[] = {
     JS_FN( "log", Log, 0, 0 ),
     JS_FS_END
};
}

namespace mozjs
{

bool DefineConsole( JSContext* cx, JS::HandleObject global )
{
     JS::RootedObject consoleObj( cx, JS_NewPlainObject( cx ) );
     return consoleObj
          && JS_DefineFunctions( cx, consoleObj, console_functions )
          && JS_DefineProperty( cx, global, "console", consoleObj, 0 );
}

}
