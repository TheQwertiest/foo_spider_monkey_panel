#include <stdafx.h>
#include "popup_msg.h"

// TODO: inspect and cleanup

namespace
{

class action : public delay_loader_action
{
public:
    action( const char* p_msg, const char* p_title )
        : msg_( p_msg )
        , title_( p_title )
    {
    }

    void execute() override
    {
        ::popup_message::g_show( msg_, title_ );
    }

private:
    pfc::string_simple msg_;
    pfc::string_simple title_;
};

} // namespace

bool delay_loader::services_initialized_ = false;
std::vector<std::unique_ptr<delay_loader_action>> delay_loader::callbacks_;

void delay_loader::g_enqueue( std::unique_ptr<delay_loader_action> callback )
{
    if ( !callback )
    {
        return;
    }

    if ( services_initialized_ )
    {
        callback->execute();
    }
    else
    {
        callbacks_.emplace_back( std::move( callback ) );
    }
}

void delay_loader::g_set_ready()
{
    services_initialized_ = true;

    for ( auto& callback : callbacks_ )
    {
        callback->execute();
    }

    callbacks_.clear();
}

void popup_msg::g_show( const char* p_msg, const char* p_title )
{
    delay_loader::g_enqueue( std::make_unique<action>( p_msg, p_title ) );
}
