#pragma once

#include <stdexcept>

namespace smp
{

/// @brief This exception should be used when JS exception is not set
class SmpException
    : public std::runtime_error
{
public:
    explicit SmpException( const std::string& errorText )
        : std::runtime_error( errorText )
    {
    }

    virtual ~SmpException() = default;

    /// @details Do not pass dynamically generated strings here (e.g. fmt::format)!
    ///          Should only be used with static strings to minimize performance impact.
     _Post_satisfies_( checkValue ) 
    static void ExpectTrue( bool checkValue, std::string_view errorMessage );

    /// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
    static void ExpectTrue( _Post_notnull_ void* checkValue, std::string_view errorMessage );

    template <typename... Args>
     _Post_satisfies_( checkValue ) 
    static void ExpectTrue( bool checkValue, std::string_view errorMessage, Args&&... errorMessageFmtArgs )
    {
        if ( !checkValue )
        {
            throw SmpException( fmt::format( errorMessage, std::forward<Args>( errorMessageFmtArgs )... ) );
        }
    }
  
    template <typename... Args>
     _Post_satisfies_( checkValue ) 
    static void ExpectTrue( bool checkValue, std::wstring_view errorMessage, Args&&... errorMessageFmtArgs )
    {
        if ( !checkValue )
        {
            const auto u8msg = smp::unicode::ToU8( fmt::format( errorMessage, std::forward<Args>( errorMessageFmtArgs )... ) );
            throw SmpException( u8msg );
        }
    }

    /// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
    template <typename StrT, typename... Args>
    static void ExpectTrue( _Post_notnull_ void* checkValue, StrT errorMessage, Args&&... errorMessageFmtArgs )
    {
        ExpectTrue( static_cast<bool>( checkValue ), errorMessage, std::forward<Args>( errorMessageFmtArgs )... );
    }
};

/// @brief This exception should be used when JS exception is set
class JsException
    : public std::exception
{
public:
    JsException() = default;
    virtual ~JsException() = default;

     _Post_satisfies_( checkValue ) 
    static void ExpectTrue( bool checkValue );

    /// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
    static void ExpectTrue( _Post_notnull_ void* checkValue );
};

} // namespace smp
