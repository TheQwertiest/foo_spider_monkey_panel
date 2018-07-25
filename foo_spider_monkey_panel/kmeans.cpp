#include "stdafx.h"
#include "kmeans.h"

#include <numeric>

/*
k-means is a clustering algorithm designed to group data points into clusters of similar points,
and return the averaged "center" value of each cluster. The algorithm runs over the data set
multiple times, assigning points to the nearest center, and then re-calculating the center values
after each iteration, until the centers stabilize of max_iterations have been run.

Detailed information here: https://en.wikipedia.org/wiki/K-means_clustering

Here it is being used to group RGB colour values into clusters of similar colours for the purposes
of generating a colour scheme from an image. Each data point is a distinct RGB value that
represents a number of pixels with the same RGB value from the original image. Therefore while
every data-point is distinct, they do not all carry the same "weight" for the purposes of
determining the center points of each cluster.

In standard k-means, the starting center values are chosen at random. This provides better results
at the expense of potentially different values on subsequent runs with the same inputs. That was
not acceptable for generating colour values, so the starting center colour values are evenly spaced
across the data set.
*/

namespace
{
uint8_t kNumberOfColourComponents = 3;
}

namespace kmeans
{

Point::Point( uint32_t id_point, const std::vector<uint32_t>& values, uint32_t pixel_count )
{
    this->id_point = id_point;
    this->pixel_count = pixel_count;
    this->values = values; 
}

uint32_t Point::getID() const
{
    return id_point;
}

void Point::setCluster( uint32_t id_cluster )
{
    this->id_cluster = id_cluster;
}

uint32_t Point::getCluster()const
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

Cluster::Cluster( uint32_t id_cluster, Point point )
{
    this->id_cluster = id_cluster;

    central_values.assign(point.getValues().cbegin(), point.getValues().cend());
    points.push_back( point );
}

void Cluster::addPoint( Point point )
{
    points.push_back( point );
}

bool Cluster::removePoint( uint32_t id_point )
{
    uint32_t total_points = points.size();

    for ( uint32_t i = 0; i < total_points; i++ )
    {
        if ( points[i].getID() == id_point )
        {
            points.erase( points.begin() + i );
            return true;
        }
    }
    return false;
}

Point Cluster::getPoint( uint32_t index ) const
{
    return points[index];
}

uint32_t Cluster::getTotalPoints() const
{
    uint32_t total = std::accumulate( points.begin(), points.end(), 0, []( auto sum, const auto& curPoint )
    {
        return sum + curPoint.getPixelCount();
    } );

    return total;
}

uint32_t Cluster::getSize() const
{
    return points.size();
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
uint32_t KMeans::getIDNearestCenter( Point point ) const
{
    double min_dist;
    uint32_t id_cluster_center = 0;

    const auto& pointValues = point.getValues();
    {// i = 0
        double sum = 0.0;
        const auto& centralValues = clusters[0].getCentralValues();
        
        sum += 2 * pow( centralValues[0] - pointValues[0], 2.0 ); // r
        sum += 4 * pow( centralValues[1] - pointValues[1], 2.0 ); // g
        sum += 3 * pow( centralValues[2] - pointValues[2], 2.0 ); // b

        min_dist = sum;
    }

    for ( uint32_t i = 1; i < K; ++i )
    {
        double dist = 0.0;
        const auto& centralValues = clusters[i].getCentralValues();

        dist += 2 * pow( centralValues[0] - pointValues[0], 2.0 );
        dist += 4 * pow( centralValues[1] - pointValues[1], 2.0 );
        dist += 3 * pow( centralValues[2] - pointValues[2], 2.0 );

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
    this->K = std::min( std::max( K, static_cast<uint32_t>(14) ), total_points );
    this->total_points = total_points;
    this->colour_components = kNumberOfColourComponents;
    this->max_iterations = max_iterations;
}

std::vector<Cluster> KMeans::run( std::vector<Point> & points )
{
    std::vector<uint32_t> prohibited_indexes;

    // choose K distinct values for the centers of the clusters
    uint32_t index_point = 0;
    for ( uint32_t i = 0; i < K; ++i )
    {
        index_point = static_cast<uint32_t>(i * total_points / K); // colours are already distinct so we can't have duplicate centers
        points[index_point].setCluster( i );
        Cluster cluster( i, points[index_point] );
        clusters.push_back( cluster );
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
                if ( id_old_cluster != -1 )
                {
                    clusters[id_old_cluster].removePoint( points[i].getID() );
                }

                points[i].setCluster( id_nearest_center );
                clusters[id_nearest_center].addPoint( points[i] );
                done = false;
            }
        }

        // recalculating the center of each cluster
        for ( uint32_t i = 0; i < K; i++ )
        {
            auto& centralValues = clusters[i].getCentralValues();
            for ( uint32_t j = 0; j < colour_components; j++ )
            {
                uint32_t total_points_cluster = clusters[i].getTotalPoints();
                double sum = 0.0;

                if ( total_points_cluster > 0 )
                {
                    for ( uint32_t p = 0; p < clusters[i].getSize(); p++ )
                    {
                        sum += clusters[i].getPoint( p ).getValues()[j] * clusters[i].getPoint( p ).getPixelCount();
                    }
                    centralValues[j] = sum / total_points_cluster;
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

}