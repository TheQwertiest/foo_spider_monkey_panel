#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_engine/native_to_js_invoker.h>

#include <optional>


class js_panel_window;

namespace mozjs
{

class JsEngine;
class JsGlobalObject;
class JsGdiGraphics;

class JsContainer final
{
    friend class JsEngine;

public:
    JsContainer();
    ~JsContainer();

public:
    enum JsStatus
    {
        Mjs_Ready,
        Mjs_NotInitialized,
        Mjs_Failed
    };

public:   
    bool Reinitialize( js_panel_window& parentPanel );
    void Finalize();

    bool ExecuteScript( std::string_view scriptCode );

    template <typename ReturnType = std::nullptr_t, typename... Args>
    std::optional<ReturnType> InvokeJsCallback( std::string_view functionName,
                                                Args&&... args )
    {
        assert( Mjs_Ready == jsStatus_ );

        return mozjs::InvokeJsCallback( pJsCtx_, jsGlobal_, functionName, args... );
    }    

    void Fail();

    JsStatus GetStatus() const;
    
    JS::HandleObject GetGraphics() const;

    class GraphicsWrapper
    {
    public:
        GraphicsWrapper( JsContainer& parent, Gdiplus::Graphics& gr );
        ~GraphicsWrapper();
    private:
        JsContainer & parent_;
    };

private:
    JsContainer( const JsContainer& ) = delete;

    bool Initialize( JSContext *cx, js_panel_window& parentPanel );

private:
    JSContext * pJsCtx_;
    JS::PersistentRootedObject jsGlobal_;
    JS::PersistentRootedObject jsGraphics_;
    JsGdiGraphics* nativeGraphics_;

    JsStatus jsStatus_;
};

}
