#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 )  
#include <mozjs/jsapi.h>
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
     void ExecuteScript( JS::HandleObject globalObject );
     void InbokeCallback();

     void CreateGlobalObject( JS::PersistentRootedObject& globalObject );
     void DestroyGlobalObject( JS::PersistentRootedObject& globalObject );

private:
     JsEngine( const JsEngine& );

private:
     void Initialize();
     void Finalize();

private:
     JSContext * pJsCtx_;

     uint32_t globalObjectCount_;
};

}