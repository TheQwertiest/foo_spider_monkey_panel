#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace mozjs
{

class JsMeasureStringInfo
{
public:
    ~JsMeasureStringInfo();

    static JSObject* Create( JSContext* cx, float x, float y, float w, float h, uint32_t lines, uint32_t characters );

    static const JSClass& GetClass();

public:
    std::optional<uint32_t> get_Chars();
    std::optional<float> get_Height();
    std::optional<uint32_t> get_Lines();
    std::optional<float> get_Width();
    std::optional<float> get_X();
    std::optional<float> get_Y();

private:
    JsMeasureStringInfo( JSContext* cx, float x, float y, float w, float h, uint32_t l, uint32_t c );
    JsMeasureStringInfo( const JsMeasureStringInfo& ) = delete;
    JsMeasureStringInfo& operator=( const JsMeasureStringInfo& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;

    float x_, y_, w_, h_;
    int lines_, characters_;
};

}
