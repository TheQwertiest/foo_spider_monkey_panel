// Based on https://github.com/marcoscastro/kmeans
// Copyright (c) 2015 Marcos Castro de Souza
// All rights reserved.

#include <stdafx.h>
#include "kmeans.h"

#include <numeric>

namespace
{
constexpr uint8_t kNumberOfColourComponents = 3;
}

namespace smp::utils::kmeans
{

Point::Point( uint32_t id, const std::vector<uint32_t>& values, uint32_t pixel_count )
    : id( id )
    , pixel_count( pixel_count )
    , values( values )
{
}

Cluster::Cluster( uint32_t id_cluster, const Point* pPoint )
    : id_cluster( id_cluster )
{
    assert( pPoint );

    central_values = pPoint->values | ranges::view::transform( []( const auto& elem ) { return static_cast<double>( elem ); } );
    points.push_back( pPoint );
}

Cluster::Cluster( Cluster&& other )
    : id_cluster( other.id_cluster )
    , central_values( std::move( other.central_values ) )
    , points( std::move( other.points ) )
{
}

Cluster& Cluster::operator=( Cluster&& other )
{
    if ( this != &other )
    {
        id_cluster = other.id_cluster;
        central_values = std::move( other.central_values );
        points = std::move( other.points );
    }

    return *this;
}

uint32_t Cluster::getTotalPixelCount() const
{
    return ranges::accumulate( points, 0, []( auto sum, const auto pPoint ) {
        return sum + pPoint->pixel_count;
    } );
}

// return ID of nearest center
// uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
uint32_t KMeans::getIDNearestCenter( const std::vector<Cluster>& clusters, const Point& point ) const
{
    const auto& pointValues = point.values;

    auto calculateDistance = [&pointValues]( const Cluster& cluster ) {
        double sum = 0.0;
        const auto& centralValues = cluster.central_values;

        sum += 2 * pow( centralValues[0] - pointValues[0], 2.0 ); // r
        sum += 4 * pow( centralValues[1] - pointValues[1], 2.0 ); // g
        sum += 3 * pow( centralValues[2] - pointValues[2], 2.0 ); // b

        return sum;
    };

    uint32_t id_cluster_center = 0;
    double min_dist = calculateDistance( clusters[0] );

    for ( uint32_t i = 1; i < clusters.size(); ++i )
    {
        double dist = calculateDistance( clusters[i] );
        if ( dist < min_dist )
        {
            min_dist = dist;
            id_cluster_center = i;
        }
    }

    return id_cluster_center;
}

KMeans::KMeans( uint32_t K, uint32_t max_iterations )
    : K( K )
    , max_iterations( max_iterations )
    , colour_components( kNumberOfColourComponents )
{
}

std::vector<Cluster> KMeans::run( std::vector<Point>& points )
{
    const size_t clusterCount = std::min( std::max( K, static_cast<uint32_t>( 14 ) ), points.size() );

    std::vector<Cluster> clusters;
    clusters.reserve( clusterCount );

    // choose K distinct values for the centers of the clusters
    for ( uint32_t i = 0; i < clusterCount; ++i )
    {
        auto& centerPoint = points[static_cast<size_t>( i * points.size() / clusterCount )]; // colours are already distinct so we can't have duplicate centers
        centerPoint.id_cluster = i;
        clusters.emplace_back( i, &centerPoint );
    }

    for ( uint32_t iter = 0; iter < max_iterations; ++iter )
    {
        bool done = true;

        // associate each point to its nearest center
        for ( auto& point : points )
        {
            uint32_t id_old_cluster = point.id_cluster;
            uint32_t id_nearest_center = getIDNearestCenter( clusters, point );

            if ( id_old_cluster != id_nearest_center )
            {
                if ( id_old_cluster != uint32_t( -1 ) )
                {
                    auto& clusterPoints = clusters[id_old_cluster].points;
                    const auto it = ranges::find_if( clusterPoints, [id_point = point.id]( const auto pPoint ) { return ( pPoint->id == id_point ); } );
                    assert( it != clusterPoints.cend() );
                    clusterPoints.erase( it );
                }

                point.id_cluster = id_nearest_center;
                clusters[id_nearest_center].points.push_back( &point );
                done = false;
            }
        }

        // recalculating the center of each cluster
        for ( auto& cluster : clusters )
        {
            auto& centralValues = cluster.central_values;
            for ( uint32_t j = 0; j < colour_components; j++ )
            {
                const uint32_t pixelsInCluster = cluster.getTotalPixelCount();
                if ( pixelsInCluster )
                {
                    const uint32_t sum = ranges::accumulate( cluster.points, 0, [j]( uint32_t curSum, const auto pPoint ) {
                        return curSum + pPoint->values[j] * pPoint->pixel_count;
                    } );
                    centralValues[j] = static_cast<double>( sum ) / pixelsInCluster;
                }
            }
        }

        if ( done )
        {
            break;
        }
    }

    return clusters;
}

} // namespace smp::utils::kmeans
