#pragma once

#include "js_value_converter.h"

#include <optional>


namespace mozjs
{

class JsEngine final
{
public:
    ~JsEngine();

    static JsEngine& GetInstance();

public:
    JSContext * GetJsContext() const;

    bool CreateGlobalObject( JS::PersistentRootedObject& globalObject );
    void DestroyGlobalObject( JS::PersistentRootedObject& globalObject );

    bool ExecuteScript( JS::HandleObject globalObject, std::string_view scriptCode );

    template <typename ReturnType = std::nullptr_t, typename... Args>
    std::optional<ReturnType> InvokeCallback( JS::HandleObject globalObject,
                                              std::string_view functionName,
                                              Args&&... args )
    {
        JS::RootedValue retVal( pJsCtx_ );

        if constexpr (sizeof...(Args) > 0)
        {
            JS::AutoValueArray<sizeof...(Args)> wrappedArgs( pJsCtx_ );
            mozjs::WrapArguments( wrappedArgs, 0, args... );

            if (!InvokeCallbackInternal( globalObject, functionName, wrappedArgs, &retVal ))
            {
                return std::nullopt;
            }
        }
        else
        {
            if (!InvokeCallbackInternal( globalObject, functionName, JS::HandleValueArray::empty(), &retVal ))
            {
                return std::nullopt;
            }
        }

        ReturnType unwrappedRetVal;
        if (mozjs::UnwrapValue( retVal, unwrappedRetVal ))
        {
            return std::nullopt;
        }

        return std::optional<ReturnType>{unwrappedRetVal};
    }

private:
    JsEngine();
    JsEngine( const JsEngine& );

private:
    bool Initialize();
    void Finalize();

    bool InvokeCallbackInternal( JS::HandleObject globalObject,
                                 std::string_view functionName,
                                 const JS::HandleValueArray& args,
                                 JS::MutableHandleValue rval );

private:
    JSContext * pJsCtx_;

    uint32_t globalObjectCount_;
};

}
