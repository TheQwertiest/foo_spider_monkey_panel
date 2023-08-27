#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

const auto kWatchedObjects = std::to_array<std::pair<GUID, EventId>>( {
    { standard_config_objects::bool_playlist_stop_after_current, EventId::kNew_FbCfgPlaylistStopAfterCurrentChanged },
    { standard_config_objects::bool_cursor_follows_playback, EventId::kNew_FbCfgCursorFollowsPlaybackChanged },
    { standard_config_objects::bool_playback_follows_cursor, EventId::kNew_FbCfgPlaybackFollowsCursorChanged },
    { standard_config_objects::bool_ui_always_on_top, EventId::kNew_FbCfgUiAlwaysOnTopChanged },
} );

}

namespace
{

class ConfigObjectNotifyImpl : public config_object_notify
{
public:
    ConfigObjectNotifyImpl();

    GUID get_watched_object( t_size p_index ) override;
    t_size get_watched_object_count() override;
    void on_watched_object_changed( const config_object::ptr& p_object ) override;
};

} // namespace

namespace
{

ConfigObjectNotifyImpl::ConfigObjectNotifyImpl()
{
}

GUID ConfigObjectNotifyImpl::get_watched_object( t_size p_index )
{
    if ( p_index >= kWatchedObjects.size() )
    {
        return pfc::guid_null;
    }

    return kWatchedObjects[p_index].first;
}

t_size ConfigObjectNotifyImpl::get_watched_object_count()
{
    return kWatchedObjects.size();
}

void ConfigObjectNotifyImpl::on_watched_object_changed( const config_object::ptr& p_object )
{
    const auto it = ranges::find_if( kWatchedObjects,
                                     [guid = p_object->get_guid()]( const auto& elem ) { return elem.first == guid; } );
    if ( kWatchedObjects.cend() == it )
    {
        return;
    }

    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( it->second ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( ConfigObjectNotifyImpl );

} // namespace
