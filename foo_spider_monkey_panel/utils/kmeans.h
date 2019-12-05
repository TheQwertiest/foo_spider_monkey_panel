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

struct PointData
{
    PointData() = default;
    PointData( const std::vector<uint8_t>& values, uint32_t pixel_count );

    std::vector<uint8_t> values;
    uint32_t pixel_count = 0;
};

struct ClusterData
{
    std::vector<uint8_t> central_values;
    std::vector<const PointData*> points;
};

std::vector<ClusterData> run( const std::vector<PointData>& points, uint32_t K, uint32_t max_iterations );

} // namespace smp::utils::kmeans
