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

Point::Point( uint32_t id_point, const std::vector<uint32_t>& values, uint32_t pixel_count )
    : id_point( id_point )
    , pixel_count( pixel_count )
    , values( values )
{
}

uint32_t Point::getID() const
{
    return id_point;
}

void Point::setCluster( uint32_t new_cluster_id )
{
    id_cluster = new_cluster_id;
}

uint32_t Point::getCluster() const
{
    return id_cluster;
}

uint32_t Point::getPixelCount() const
{
    return pixel_count;
}

const std::vector<uint32_t>& Point::getValues() const
{
    return values;
}

Cluster::Cluster( uint32_t id_cluster, const Point& point )
{
    this->id_cluster = id_cluster;

    central_values = point.getValues() | ranges::view::transform( []( const auto& elem ) { return static_cast<double>( elem ); } );
    points.push_back( point );
}

bool Cluster::removePoint( uint32_t id_point )
{
    for ( auto it = points.cbegin(); it != points.cend(); ++it )
    {
        if ( it->getID() == id_point )
        {
            points.erase( it );
            return true;
        }
    }
    return false;
}

uint32_t Cluster::getTotalPixelCount() const
{
    return ranges::accumulate( points, 0, []( auto sum, const auto& curPoint ) {
        return sum + curPoint.getPixelCount();
    } );
}

const std::vector<Point>& Cluster::getPoints() const
{
    return points;
}

std::vector<Point>& Cluster::getPoints()
{
    return points;
}

const std::vector<double>& Cluster::getCentralValues() const
{
    return central_values;
}

std::vector<double>& Cluster::getCentralValues()
{
    return central_values;
}

// return ID of nearest center
// uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
uint32_t KMeans::getIDNearestCenter( const Point& point ) const
{
    const auto& pointValues = point.getValues();

    auto calculateDistance = [&]( uint32_t idx ) {
        double sum = 0.0;
        const auto& centralValues = clusters[idx].getCentralValues();

        sum += 2 * pow( centralValues[0] - pointValues[0], 2.0 ); // r
        sum += 4 * pow( centralValues[1] - pointValues[1], 2.0 ); // g
        sum += 3 * pow( centralValues[2] - pointValues[2], 2.0 ); // b

        return sum;
    };

    uint32_t id_cluster_center = 0;
    double min_dist = calculateDistance( 0 );

    for ( uint32_t i = 1; i < K; ++i )
    {
        double dist = calculateDistance( i );
        if ( dist < min_dist )
        {
            min_dist = dist;
            id_cluster_center = i;
        }
    }

    return id_cluster_center;
}

KMeans::KMeans( uint32_t K, uint32_t total_points, uint32_t max_iterations )
{
    this->K = std::min( std::max( K, static_cast<uint32_t>( 14 ) ), total_points );
    this->total_points = total_points;
    this->colour_components = kNumberOfColourComponents;
    this->max_iterations = max_iterations;
}

std::vector<Cluster> KMeans::run( std::vector<Point>& points )
{
    std::vector<uint32_t> prohibited_indexes;

    // choose K distinct values for the centers of the clusters
    for ( uint32_t i = 0; i < K; ++i )
    {
        uint32_t index_point = static_cast<uint32_t>( i * total_points / K ); // colours are already distinct so we can't have duplicate centers
        points[index_point].setCluster( i );
        clusters.emplace_back( i, points[index_point] );
    }

    uint32_t iter = 1;

    while ( true )
    {
        bool done = true;

        // associate each point to its nearest center
        for ( uint32_t i = 0; i < total_points; i++ )
        {
            uint32_t id_old_cluster = points[i].getCluster();
            uint32_t id_nearest_center = getIDNearestCenter( points[i] );

            if ( id_old_cluster != id_nearest_center )
            {
                if ( id_old_cluster != uint32_t( -1 ) )
                {
                    clusters[id_old_cluster].removePoint( points[i].getID() );
                }

                points[i].setCluster( id_nearest_center );
                clusters[id_nearest_center].getPoints().push_back( points[i] );
                done = false;
            }
        }

        // recalculating the center of each cluster
        for ( uint32_t i = 0; i < K; i++ )
        {
            auto& centralValues = clusters[i].getCentralValues();
            for ( uint32_t j = 0; j < colour_components; j++ )
            {
                uint32_t pixelsInCluster = clusters[i].getTotalPixelCount();
                if ( pixelsInCluster )
                {
                    const auto& clPoints = clusters[i].getPoints();
                    double sum = 0.0;
                    for ( uint32_t p = 0; p < clPoints.size(); p++ )
                    {
                        sum += clPoints[p].getValues()[j] * clPoints[p].getPixelCount();
                    }
                    centralValues[j] = sum / pixelsInCluster;
                }
            }
        }

        if ( done || iter >= max_iterations )
        {
            break;
        }

        iter++;
    }

    return clusters;
}

} // namespace smp::utils::kmeans
