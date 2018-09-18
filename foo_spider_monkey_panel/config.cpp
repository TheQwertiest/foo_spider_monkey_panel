#include "stdafx.h"
#include "config.h"
#include "resource.h"

namespace smp::config
{

DWORD edge_style_from_config( EdgeStyle edge_style )
{
    switch ( edge_style )
    {
        case EdgeStyle::SUNKEN_EDGE:
            return WS_EX_CLIENTEDGE;
        case EdgeStyle::GREY_EDGE:
            return WS_EX_STATICEDGE;
        default:
            return 0;
    }
}

PanelProperties::config_map& PanelProperties::get_val()
{
    return m_map;
}

std::optional<mozjs::SerializedJsValue> PanelProperties::get_config_item( const std::wstring& propName )
{
    if ( !m_map.count( propName ) )
    {
        return std::nullopt;
    }

    return *( m_map[propName].get() );
}

void PanelProperties::set_config_item( const std::wstring& propName, const mozjs::SerializedJsValue& serializedValue )
{
    m_map.insert_or_assign( propName, std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
}

void PanelProperties::remove_config_item( const std::wstring& propName )
{
    m_map.erase( propName );
}

void PanelProperties::g_load( config_map& data, stream_reader* reader, abort_callback& abort ) throw()
{
    data.clear();

    try
    {
        uint32_t count;
        reader->read_lendian_t( count, abort );

        for ( uint32_t i = 0; i < count; ++i )
        {
            mozjs::SerializedJsValue serializedValue;

            pfc::string8_fast pfcPropName;
            reader->read_string( pfcPropName, abort );

            uint32_t valueType;
            reader->read_lendian_t( valueType, abort );
            serializedValue.type = static_cast<mozjs::JsValueType>( valueType );

            switch ( serializedValue.type )
            {
                case mozjs::JsValueType::pt_boolean:
                {
                    reader->read_lendian_t( serializedValue.boolVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_int32:
                {
                    reader->read_lendian_t( serializedValue.intVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_double:
                {
                    reader->read_lendian_t( serializedValue.doubleVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_string:
                {
                    pfc::string8_fast pfcStrVal;
                    reader->read_string( pfcStrVal, abort );
                    serializedValue.strVal.add_string_nc( pfcStrVal.c_str(), pfcStrVal.length() );
                    break;
                }
                default:
                    assert( 0 );
                    break;
            }

            pfc::stringcvt::string_wide_from_utf8 propnameW( pfcPropName.c_str(), pfcPropName.length() );
            data.emplace( propnameW.get_ptr(), std::make_shared<mozjs::SerializedJsValue>( serializedValue ) );
        }
    }
    catch ( ... )
    {
    }
}

void PanelProperties::g_save( const config_map& data, stream_writer* writer, abort_callback& abort ) throw()
{
    try
    {
        writer->write_lendian_t( static_cast<uint32_t>( data.size() ), abort );

        for ( const auto& [name, pValue] : data )
        {
            pfc::stringcvt::string_utf8_from_wide propNameW( name.c_str(), name.length() );
            writer->write_string( propNameW.get_ptr(), propNameW.length(), abort );

            const auto& serializedValue = *pValue;

            uint32_t valueType = static_cast<uint32_t>( serializedValue.type );
            writer->write_lendian_t( valueType, abort );

            switch ( serializedValue.type )
            {
                case mozjs::JsValueType::pt_boolean:
                {
                    writer->write_lendian_t( serializedValue.boolVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_int32:
                {
                    writer->write_lendian_t( serializedValue.intVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_double:
                {
                    writer->write_lendian_t( serializedValue.doubleVal, abort );
                    break;
                }
                case mozjs::JsValueType::pt_string:
                {
                    writer->write_string( serializedValue.strVal.c_str(), serializedValue.strVal.length(), abort );
                    break;
                }
                default:
                    assert( 0 );
                    break;
            }
        }
    }
    catch ( ... )
    {
    }
}

void PanelProperties::load( stream_reader* reader, abort_callback& abort ) throw()
{
    g_load( m_map, reader, abort );
}

void PanelProperties::save( stream_writer* writer, abort_callback& abort ) const throw()
{
    g_save( m_map, writer, abort );
}

PanelSettings::PanelSettings()
{
    reset_config();
}

GUID& PanelSettings::get_config_guid()
{
    return m_config_guid;
}

WINDOWPLACEMENT& PanelSettings::get_windowplacement()
{
    return m_wndpl;
}

bool& PanelSettings::get_grab_focus()
{
    return m_grab_focus;
}

bool& PanelSettings::get_pseudo_transparent()
{
    return m_pseudo_transparent;
}

const bool& PanelSettings::get_pseudo_transparent() const
{
    return m_pseudo_transparent;
}

const EdgeStyle& PanelSettings::get_edge_style() const
{
    return m_edge_style;
}

pfc::string_base& PanelSettings::get_script_code()
{
    return m_script_code;
}

PanelProperties& PanelSettings::get_config_prop()
{
    return m_config_prop;
}

pfc::string8_fast PanelSettings::get_default_script_code()
{
    pfc::string8_fast scriptCode;
    puResource pures = uLoadResource( core_api::get_my_instance(), uMAKEINTRESOURCE( IDR_SCRIPT ), "SCRIPT" );
    if ( pures )
    {
        scriptCode.set_string( reinterpret_cast<const char*>( pures->GetPointer() ), pures->GetSize() );
    }
    return scriptCode;
}

EdgeStyle& PanelSettings::get_edge_style()
{
    return m_edge_style;
}

void PanelSettings::load_config( stream_reader* reader, t_size size, abort_callback& abort )
{
    reset_config();

    if ( size > sizeof( Version ) )
    {
        try
        {
            uint32_t ver = 0;
            reader->read_object_t( ver, abort );
            if ( ver > static_cast<uint32_t>( Version::CONFIG_VERSION_CURRENT ) )
            {
                throw std::runtime_error( "" );
            }
            reader->skip_object( sizeof( false ), abort ); // HACK: skip over "delay load"
            reader->read_object_t( m_config_guid, abort );
            reader->read_object( &m_edge_style, sizeof( m_edge_style ), abort );
            m_config_prop.load( reader, abort );
            reader->skip_object( sizeof( false ), abort ); // HACK: skip over "disable before"
            reader->read_object_t( m_grab_focus, abort );
            reader->read_object( &m_wndpl, sizeof( m_wndpl ), abort );
            reader->read_string( m_script_code, abort );
            reader->read_object_t( m_pseudo_transparent, abort );
        }
        catch ( ... )
        {
            reset_config();
            FB2K_console_formatter() << "Error: " SMP_NAME_WITH_VERSION " Configuration has been corrupted. All settings have been reset.";
        }
    }
}

void PanelSettings::reset_config()
{
    m_script_code = get_default_script_code();
    m_pseudo_transparent = false;
    m_wndpl.length = 0;
    m_grab_focus = true;
    m_edge_style = EdgeStyle::NO_EDGE;
    CoCreateGuid( &m_config_guid );
}

void PanelSettings::save_config( stream_writer* writer, abort_callback& abort ) const
{
    try
    {
        auto currentVersion = static_cast<uint32_t>( Version::CONFIG_VERSION_CURRENT );
        writer->write_object_t( currentVersion, abort );
        writer->write_object_t( false, abort ); // HACK: write this in place of "delay load"
        writer->write_object_t( m_config_guid, abort );
        writer->write_object( &m_edge_style, sizeof( m_edge_style ), abort );
        m_config_prop.save( writer, abort );
        writer->write_object_t( false, abort ); // HACK: write this in place of "disable before"
        writer->write_object_t( m_grab_focus, abort );
        writer->write_object( &m_wndpl, sizeof( m_wndpl ), abort );
        writer->write_string( m_script_code, abort );
        writer->write_object_t( m_pseudo_transparent, abort );
    }
    catch ( ... )
    {
    }
}

} // namespace smp::config