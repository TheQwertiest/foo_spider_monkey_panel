#pragma once

#include <optional>
#include <string>

namespace smp::version
{

/// @brief See https://semver.org/
class SemVer
{
public:
    SemVer() = default;
    /// @throw std::runtime_error if parsing failed
    SemVer( const std::string& strVer );

    static std::optional<SemVer> ParseString( const std::string& strVer );

    bool operator==( const SemVer& other ) const;
    bool operator!=( const SemVer& other ) const;
    bool operator<( const SemVer& other ) const;
    bool operator>( const SemVer& other ) const;
    bool operator<=( const SemVer& other ) const;
    bool operator>=( const SemVer& other ) const;

private:
    static bool IsPreleaseNewer( std::string_view a, std::string_view b );

public:
    uint8_t major = 0;
    uint8_t minor = 0;
    uint8_t patch = 0;
    std::string prerelease;
    std::string metadata;
};

} // namespace smp::version
