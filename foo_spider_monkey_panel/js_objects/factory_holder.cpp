#include <stdafx.h>
#include "factory_holder.h"


namespace mozjs
{

JsFactoryHolder::JsFactoryHolder()
{

}

JsFactoryHolder::~JsFactoryHolder()
{

}

JsFactoryHolder& JsFactoryHolder::GetInstance()
{
    static JsFactoryHolder factoryHolder;
    return factoryHolder;
}

}
