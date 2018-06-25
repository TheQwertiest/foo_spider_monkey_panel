#pragma once

#include "js_value_converter.h"
#include <js_objects/js_object_wrapper.h>
#include <js_objects/global_object.h>

#include <optional>
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

    template <typename ReturnType = std::nullptr_t, typename... Args>
    std::optional<ReturnType> InvokeCallback( JS::HandleObject globalObject,
                                              std::string_view functionName,
                                              Args&&... args )
    {
        JS::RootedValue retVal( pJsCtx_ );

        if constexpr ( sizeof...( Args ) > 0 )
        {
            JS::AutoValueArray<sizeof...( Args )> wrappedArgs( pJsCtx_ );
            if ( !NativeToJsArguments( pJsCtx_, wrappedArgs, 0, args... ) )
            {
                return std::nullopt;
            }

            if ( !InvokeCallbackInternal( globalObject, functionName, wrappedArgs, &retVal ) )
            {
                return std::nullopt;
            }
        }
        else
        {
            if ( !InvokeCallbackInternal( globalObject, functionName, JS::HandleValueArray::empty(), &retVal ) )
            {
                return std::nullopt;
            }
        }

        if ( !JsToNative<ReturnType>::IsValid( pJsCtx_, retVal ) )
        {
            return std::nullopt;
        }

        return JsToNative<ReturnType>::Convert( pJsCtx_, retVal );
    }

private:
    JsEngine();
    JsEngine( const JsEngine& ) = delete;

private:
    bool Initialize();
    void Finalize();

    bool InvokeCallbackInternal( JS::HandleObject globalObject,
                                 std::string_view functionName,
                                 const JS::HandleValueArray& args,
                                 JS::MutableHandleValue rval );


    template <int ArgArraySize, typename ArgType, typename... Args>
    bool NativeToJsArguments( JSContext * cx, 
                              JS::AutoValueArray<ArgArraySize>& wrappedArgs, 
                              uint8_t argIndex, ArgType arg, Args&&... args )
    {
        return NativeToJsValue( cx, arg, wrappedArgs[argIndex] )
            && NativeToJsArguments( cx, wrappedArgs, argIndex + 1, args... );
    }

    template <int ArgArraySize>
    bool NativeToJsArguments( JSContext * cx, 
                              JS::AutoValueArray<ArgArraySize>& wrappedArgs, 
                              uint8_t argIndex )
    {
        return true;
    }

private:
    JSContext * pJsCtx_;

    std::set<HWND> registeredPanels_;
};

}
