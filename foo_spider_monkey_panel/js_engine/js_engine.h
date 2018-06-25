#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <set>

namespace mozjs
{

class JsEngine final
{
public:
    ~JsEngine();

    static JsEngine& GetInstance();

public:
    JSContext * GetJsContext() const;

    bool RegisterPanel( HWND hPanel );
    void UnregisterPanel( HWND hPanel );

    bool ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode );

private:
    JsEngine();
    JsEngine( const JsEngine& ) = delete;

private:
    bool Initialize();
    void Finalize();

private:
    JSContext * pJsCtx_;

    std::set<HWND> registeredPanels_;
};

}
