#pragma once

#include <variant>

namespace smp::custom_meta
{

enum class FieldValueType
{
    kUInt32,
    kString,
};

struct FieldInfo
{
    qwr::u8string metaName;
    qwr::u8string displayedName;
    FieldValueType valueType;
    bool isSummable = false;
};

using FieldValue = std::variant<uint32_t, qwr::u8string>;
using FieldValueMap = std::unordered_map<qwr::u8string, FieldValue>;
} // namespace smp::custom_meta

namespace smp::custom_meta
{
[[nodiscard]] std::optional<FieldInfo> GetFieldInfo( const qwr::u8string& fieldName );

[[nodiscard]] FieldValueMap GetData( metadb_handle_ptr handle );
void SetData( metadb_handle_ptr handle, const FieldValueMap& fields );
void SetData( const metadb_handle_list& handles, const FieldValueMap& fields );
void RefreshData( metadb_handle_ptr handle );
void RefreshData( const metadb_handle_list& handles );

} // namespace smp::custom_meta
