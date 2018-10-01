#include "stdafx.h"

#include <acfu-sdk/acfu.h>
#include <acfu-sdk/utils/github.h>

namespace smp::acfu
{

class SmpSource
    : public ::acfu::source
    , public ::acfu::github_conf
{
public:
    static pfc::string8 FetchVersion()
    {
        pfc::string8 version;

        componentversion::ptr cv;
        ::acfu::for_each_service<componentversion>( [&]( auto& ptr ) {
            pfc::string8 file_name;
            ptr->get_file_name( file_name );
            if ( file_name == componentFileName_ )
            {
                cv = ptr;
            }
        } );
        if ( cv.is_empty() )
        {
            return version;
        }

        cv->get_component_version( version );
        return version;
    }
    virtual GUID get_guid()
    {
        return g_guid_acfu_source;
    }
    virtual void get_info( file_info& info )
    {
        if ( !isVersionFetched_ )
        {
            installedVersion_ = FetchVersion();
            isVersionFetched_ = true;
        }

        if ( installedVersion_.is_empty() )
        {
            info.meta_set( "version", "0.0.0" );
        }
        else
        {
            info.meta_set( "version", installedVersion_ );
        }
        info.meta_set( "name", "Spider Monkey Panel" );
        info.meta_set( "module", componentFileName_ );
    }
    virtual bool is_newer( const file_info& info )
    {
        const std::string_view available = info.meta_get( "version", 0 );
        const std::string_view installed = installedVersion_;

        return available > installed;
    }
    virtual ::acfu::request::ptr create_request()
    {
        return new service_impl_t<::acfu::github_latest_release<SmpSource>>();
    }
    static const char* get_owner()
    {
        return "TheQwertiest";
    }
    static const char* get_repo()
    {
        return componentFileName_;
    }

private:
    static constexpr char componentFileName_[] = "foo_spider_monkey_panel";
    bool isVersionFetched_ = false;
    pfc::string8 installedVersion_;
};
static service_factory_t<SmpSource> g_smpSource;

} // namespace smp::acfu
