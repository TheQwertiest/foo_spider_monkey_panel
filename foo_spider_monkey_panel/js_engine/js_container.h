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
    // To access preparation call
    friend class JsEngine;

public:
    JsContainer();
    ~JsContainer();

public:
    enum class JsStatus
    {
        Ready,
        Prepared,
        NotPrepared,
        Failed
    };

public:   
    bool Initialize();
    void Finalize();

    bool ExecuteScript( std::string_view scriptCode );

    template <typename ReturnType = std::nullptr_t, typename... Args>
    std::optional<ReturnType> InvokeJsCallback( std::string_view functionName,
                                                Args&&... args )
    {
        if ( JsStatus::Ready != jsStatus_ )
        {
            return std::nullopt;
        }        
        return mozjs::InvokeJsCallback<ReturnType>( pJsCtx_, jsGlobal_, functionName, args... );
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

    bool Prepare( JSContext *cx, js_panel_window& parentPanel );

private:
    JSContext * pJsCtx_ = nullptr;
    js_panel_window* pParentPanel_ = nullptr;

    JS::PersistentRootedObject jsGlobal_;
    JS::PersistentRootedObject jsGraphics_;
    JsGlobalObject* pNativeGlobal_ = nullptr;
    JsGdiGraphics* pNativeGraphics_ = nullptr;

    JsStatus jsStatus_ = JsStatus::NotPrepared;
};

}
