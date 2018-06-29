#pragma once

#include <js_utils/non_copyable.h>
#include <js_objects/object_factory.h>
#include <js_objects/gdi_font.h>

namespace mozjs
{

class JsFactoryHolder
    : public Mjs_NonCopyable
{
public:
    ~JsFactoryHolder();

    static JsFactoryHolder& GetInstance();

    template <typename T>
    typename std::enable_if_t<std::is_same_v<T, JsGdiFont>, JsObjectFactory<typename T::InfoType>&>
        Factory()
    {
        return gdiFont_;
    }

private:
    JsFactoryHolder();

private:
    JsObjectFactory<JsGdiFont::InfoType> gdiFont_;
};

}
