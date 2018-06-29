#pragma once


namespace mozjs
{

class Mjs_NonCopyable
{
public:
    Mjs_NonCopyable()
    {
    };
    virtual ~Mjs_NonCopyable()
    {
    };
private:
    Mjs_NonCopyable( const Mjs_NonCopyable& ) = delete;
    Mjs_NonCopyable& operator=( const Mjs_NonCopyable& ) = delete;
};

}
