#pragma once;

struct JSErrorFormatString;

namespace mozjs
{

enum Mjs_Status
{
    Mjs_Ok = 0,
    Mjs_InvalidArgumentType,
    Mjs_InvalidArgumentCount,
    Mjs_InvalidArgumentValue,
    Mjs_InternalError,
    Mjs_EngineInternalError,
    Mjs_EnumEnd
};

const char* ErrorCodeToString( Mjs_Status errorCode);

}
