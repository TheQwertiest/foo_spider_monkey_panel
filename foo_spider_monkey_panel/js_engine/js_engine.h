#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

class JsEngine final
{
public:
     JsEngine();
     ~JsEngine();

     static JsEngine& GetInstance();

public:
     JSContext * GetJsContext() const;

     bool CreateGlobalObject( JS::PersistentRootedObject& globalObject );
     void DestroyGlobalObject( JS::PersistentRootedObject& globalObject );

     bool ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode );
     bool InbokeCallback( std::string_view functionName, JS::HandleObject globalObject, 
                          const JS::HandleValueArray& args, JS::MutableHandleValue rval );

private:
     JsEngine( const JsEngine& );

private:
     bool Initialize();
     void Finalize();

private:
     JSContext * pJsCtx_;

     uint32_t globalObjectCount_;
};

}
