#pragma once

#include <shlwapi.h>

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace smp
{

template <typename T>
class RelativeFilepathTrie
{
private:
    static_assert( std::is_trivial_v<T> );

    struct StrLogicalComparator
    {
        bool operator()( const std::wstring_view& a, const std::wstring_view& b ) const
        {
            return ( StrCmpLogicalW( a.data(), b.data() ) < 0 );
        }
    };

    struct Node;
    using NodeMap = std::unordered_map<std::wstring, std::unique_ptr<Node>>;

    struct Node
    {
        NodeMap nodes;
        std::vector<T> values;
    };

public:
    void emplace( std::wstring_view path, T value )
    {
        const auto processSuffix = []( NodeMap& nodes, std::wstring_view& suffix ) {
            const auto prefix = ExtractPrefix( suffix );
            const std::wstring prefixStr{ prefix.data(), prefix.size() };

            auto it = nodes.find( prefixStr );
            if ( it == nodes.end() )
            {
                bool dummy;
                std::tie( it, dummy ) = nodes.try_emplace( prefixStr, std::make_unique<Node>() );
            }

            return it;
        };

        // empty path is allowed, so special case it
        std::wstring_view suffix = path;
        auto it = processSuffix( nodes_, suffix );

        while ( !suffix.empty() )
        {
            it = processSuffix( it->second->nodes, suffix );
        }

        it->second->values.emplace_back( value );
        ++size_;
    }

    [[nodiscard]] size_t size() const
    {
        return size_;
    }

    [[nodiscard]] bool empty() const
    {
        return !size_;
    }

    template <typename SorterT = StrLogicalComparator>
    [[nodiscard]] std::vector<T> get_sorted_values() const
    {
        std::vector<T> values;
        values.reserve( size_ );

        FillSortedValuesRecur<SorterT>( nodes_, values );

        return values;
    }

private:
    static [[nodiscard]] std::wstring_view ExtractPrefix( std::wstring_view& str )
    {
        if ( str.empty() )
        { // "{}{}"
            return str;
        }

        const auto separatorStartPos = str.find_first_of( L"\\/" );
        if ( separatorStartPos == std::wstring::npos )
        { // "{a}{}"
            const auto prefix = str;
            str.remove_prefix( str.size() );
            return prefix;
        }

        const auto postfixStartPos = str.find_first_not_of( L"\\/", separatorStartPos );
        if ( postfixStartPos == std::wstring::npos )
        {                    // "{a}/{}"
            assert( false ); // should not reach here, since this is not a valid file path
            const auto prefix = str.substr( 0, separatorStartPos );
            str.remove_prefix( str.size() );
            return prefix;
        }
        const auto separatorEndPos = postfixStartPos - 1;

        // "{a}/{b}", "{}/{a}", "{a}///{b}"
        const auto prefix = str.substr( 0, separatorStartPos );
        str.remove_prefix( separatorEndPos + 1 );
        return prefix;
    }

    template <typename SorterT>
    static void FillSortedValuesRecur( const NodeMap& nodes, std::vector<T>& values )
    {
        // using multimap for the case, when sorter is not case sensitive
        std::multimap<std::wstring_view, Node*, SorterT> sortedNodes;
        for ( const auto& [key, pNode]: nodes )
        {
            sortedNodes.emplace( key, pNode.get() );
        }

        for ( const auto& [key, pNode]: sortedNodes )
        {
            assert( pNode );
            if ( pNode->nodes.empty() )
            {
                assert( !pNode->values.empty() );
                values.insert( values.end(), pNode->values.begin(), pNode->values.end() );
            }
            else
            {
                FillSortedValuesRecur<SorterT>( pNode->nodes, values );
            }
        }
    }

private:
    NodeMap nodes_;
    size_t size_ = 0;
};

} // namespace smp
