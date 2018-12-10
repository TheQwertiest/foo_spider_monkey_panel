#pragma once

class delay_loader_action
{
public:
    virtual void execute() = 0;
};

/// @details Class usage is not thread-safe
class delay_loader
{
public:
    static void g_enqueue( std::unique_ptr<delay_loader_action> callback );
    static void g_set_ready();

private:
    static std::vector<std::unique_ptr<delay_loader_action>> callbacks_;
    static bool services_initialized_;
};

class popup_msg
{
public:
    static void g_show( const char* p_msg, const char* p_title );
};
