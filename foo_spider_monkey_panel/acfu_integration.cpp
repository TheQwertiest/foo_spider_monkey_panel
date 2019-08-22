#include <stdafx.h>

#include <utils/string_helpers.h>
#include <utils/semantic_version.h>
#include <utils/acfu_github.h>

#include <acfu-sdk/utils/common.h>

using namespace smp::acfu;

namespace
{

class SmpSource
    : public ::acfu::source
    , public smp::acfu::github_conf
{
public:
    GUID get_guid() override;
    void get_info( file_info& info ) override;
    bool is_newer( const file_info& info ) override;
    ::acfu::request::ptr create_request() override;

    static pfc::string8 get_owner();
    static pfc::string8 get_repo();

private:
    static std::string FetchVersion();

private:
    static constexpr char componentName_[] = SMP_NAME;
    static constexpr char componentFileName_[] = SMP_UNDERSCORE_NAME;

    bool isVersionFetched_ = false;
    std::string installedVersion_;
};

} // namespace

namespace
{

GUID SmpSource::get_guid()
{
    return smp::guid::acfu_source;
}

void SmpSource::get_info( file_info& info )
{
    if ( !isVersionFetched_ )
    {
        installedVersion_ = FetchVersion();
        isVersionFetched_ = true;
    }

    info.meta_set( "version", installedVersion_.c_str() );
    info.meta_set( "name", componentName_ );
    info.meta_set( "module", componentFileName_ );
}

bool SmpSource::is_newer( const file_info& info )
{
    if ( !info.meta_get( "version", 0 ) || installedVersion_.empty() )
    {
        return false;
    }

    std::string available = info.meta_get( "version", 0 );
    available = smp::string::Trim( available );
    if ( available[0] == 'v' )
    {
        available.assign( available.c_str() + 1, available.length() - 1 );
    }

    try
    {
        return ( smp::version::SemVer{ available } > smp::version::SemVer{ installedVersion_ } );
    }
    catch ( const std::runtime_error& )
    {
        assert( false );
        return false;
    }
}

::acfu::request::ptr SmpSource::create_request()
{
    return fb2k::service_new<smp::acfu::github_latest_release<SmpSource>>();
}

pfc::string8 SmpSource::get_owner()
{
    return "TheQwertiest";
}

pfc::string8 SmpSource::get_repo()
{
    return componentFileName_;
}

std::string SmpSource::FetchVersion()
{
    auto cvRet = []() -> std::optional<componentversion::ptr> {
        for ( service_enum_t<componentversion> e; !e.finished(); ++e )
        {
            auto cv = e.get();

            pfc::string8 file_name;
            cv->get_file_name( file_name );
            if ( file_name.equals( componentFileName_ ) )
            {
                return cv;
            }
        }

        return std::nullopt;
    }();

    if ( !cvRet )
    {
        return "0.0.0";
    }
    else
    {
        pfc::string8 version;
        cvRet.value()->get_component_version( version );
        return std::string( version.c_str(), version.length() );
    }
}

service_factory_single_t<SmpSource> g_smpSource;

} // namespace
