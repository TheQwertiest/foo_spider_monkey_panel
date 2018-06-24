#include <stdafx.h>

#include "js_error_codes.h"


namespace mozjs
{

const char* ErrorCodeToString( Mjs_Status errorCode )
{
    switch ( errorCode )
    {
    case Mjs_Ok:
    {
        return "No error";
    }
    case Mjs_InvalidArgumentType:
    {
        return "Argument has invalid type";
    }
    case Mjs_InvalidArgumentCount:
    {
        return "Invalid number of arguments";
    }
    case Mjs_InvalidArgumentValue:
    {
        return "Invalid argument value";
    }
    case Mjs_InternalError:
    {
        return "Internal error";
    }
    case Mjs_EngineInternalError:
    {
        return "Internal engine error";
    }    
    default:
        return "";
    }
}

}
