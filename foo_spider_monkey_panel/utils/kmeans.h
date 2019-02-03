// Based on https://github.com/marcoscastro/kmeans
// Copyright (c) 2015 Marcos Castro de Souza
// All rights reserved.

#pragma once

#include <vector>

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

namespace smp::utils::kmeans
{

struct Point
{
    Point( uint32_t id, const std::vector<uint32_t>& values, uint32_t pixel_count );

    const uint32_t id;
    const uint32_t total_values = 0;
    const uint32_t pixel_count = 0;    
    const std::vector<uint32_t> values;

    uint32_t id_cluster = uint32_t( -1 );
};

struct Cluster
{
    Cluster( uint32_t id_cluster, const Point* pPoint );
    Cluster( Cluster&& other );
    Cluster& operator=( Cluster&& other );

    Cluster( const Cluster& other ) = delete;
    Cluster& operator=( const Cluster& other ) = delete;

    uint32_t getTotalPixelCount() const;

    uint32_t id_cluster;
    std::vector<double> central_values;
    std::vector<const Point*> points;
};

class KMeans
{
public:
    KMeans( uint32_t K, uint32_t max_iterations );

    std::vector<Cluster> run( std::vector<Point>& points );

private:
    // return ID of nearest center
    // uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
    uint32_t getIDNearestCenter( const std::vector<Cluster>& clusters, const Point& point ) const;

private:
    const uint32_t K;
    const uint32_t max_iterations;
    const uint32_t colour_components;
};

}