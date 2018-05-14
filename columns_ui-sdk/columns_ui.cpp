#include "ui_extension.h"

namespace cui {
namespace fcl {
// {04FC2444-4591-4f33-AE93-D61DF3DC32AD}
const GUID dataset::class_guid = {0x4fc2444, 0x4591, 0x4f33, {0xae, 0x93, 0xd6, 0x1d, 0xf3, 0xdc, 0x32, 0xad}};
// {78DEEA88-A3CB-47c7-95DE-39D6C81509E9}
const GUID group::class_guid = {0x78deea88, 0xa3cb, 0x47c7, {0x95, 0xde, 0x39, 0xd6, 0xc8, 0x15, 0x9, 0xe9}};
} // namespace fcl

// {28992DF0-1EE2-4e06-AC86-BF4F266E2E56}
const GUID control::class_guid = {0x28992df0, 0x1ee2, 0x4e06, {0xac, 0x86, 0xbf, 0x4f, 0x26, 0x6e, 0x2e, 0x56}};

// {EC705736-4C68-44fe-B240-74793B40FFC0}
const GUID colours::manager::class_guid
    = {0xec705736, 0x4c68, 0x44fe, {0xb2, 0x40, 0x74, 0x79, 0x3b, 0x40, 0xff, 0xc0}};

// {E1979E50-B496-47e9-88CC-04FFF19024D0}
const GUID colours::manager_instance::class_guid
    = {0xe1979e50, 0xb496, 0x47e9, {0x88, 0xcc, 0x4, 0xff, 0xf1, 0x90, 0x24, 0xd0}};

// {05B277E9-83C3-4f75-B92F-66C1049D0B68}
const GUID fonts::manager::class_guid = {0x5b277e9, 0x83c3, 0x4f75, {0xb9, 0x2f, 0x66, 0xc1, 0x4, 0x9d, 0xb, 0x68}};

// {A4AE2971-64C8-4118-8E2A-EFDC37A59372}
const GUID colours::client::class_guid = {0xa4ae2971, 0x64c8, 0x4118, {0x8e, 0x2a, 0xef, 0xdc, 0x37, 0xa5, 0x93, 0x72}};

// {3FBCC2B0-978E-406f-A446-4677E0D4C58E}
const GUID fonts::client::class_guid = {0x3fbcc2b0, 0x978e, 0x406f, {0xa4, 0x46, 0x46, 0x77, 0xe0, 0xd4, 0xc5, 0x8e}};

void cui::fcl::dataset::get_data_to_array(pfc::array_t<uint8_t>& p_data, t_uint32 type, t_export_feedback& feedback,
    abort_callback& p_abort, bool b_reset) const
{
    stream_writer_memblock_ref writer(p_data, b_reset);
    get_data(&writer, type, feedback, p_abort);
}

void cui::fcl::dataset::set_data_from_ptr(
    const void* p_data, t_size size, t_uint32 type, t_import_feedback& feedback, abort_callback& p_abort)
{
    stream_reader_memblock_ref reader(p_data, size);
    return set_data(&reader, size, type, feedback, p_abort);
}

bool fonts::client::create_by_guid(const GUID& p_guid, client::ptr& p_out)
{
    service_enum_t<client> p_enum;
    client::ptr ptr;
    while (p_enum.next(ptr)) {
        if (ptr->get_client_guid() == p_guid) {
            p_out = ptr;
            return true;
        }
    }
    return false;
}
} // namespace cui
