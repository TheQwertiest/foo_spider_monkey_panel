#pragma once

namespace mozjs::traits
{

template <typename T>
concept HasPostCreate = T::HasPostCreate || false;

template <typename T>
concept HasProxy = T::HasProxy || false;

template <typename T>
concept HasParentProto = T::HasParentProto || false;

template <typename T>
concept IsExtendable = T::IsExtendable || false;

template <typename T>
concept HasJsFunctionsMember = requires( T t )
{
    {
        t.JsFunctions
        }
        -> std::same_as<const JSFunctionSpec*&>;
};

template <typename T>
concept HasJsFunctions = HasJsFunctionsMember<T> || false;

template <typename T>
concept HasJsStaticFunctionsMember = requires( T t )
{
    {
        t.JsStaticFunctions
        }
        -> std::same_as<const JSFunctionSpec*&>;
};

template <typename T>
concept HasJsStaticFunctions = HasJsStaticFunctionsMember<T> || false;

template <typename T>
concept HasJsPropertiesMember = requires( T t )
{
    {
        t.JsProperties
        }
        -> std::same_as<const JSPropertySpec*&>;
};

template <typename T>
concept HasJsProperties = HasJsPropertiesMember<T> || false;

} // namespace mozjs::traits
