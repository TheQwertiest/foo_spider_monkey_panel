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

namespace kmeans
{

class Point
{
private:
    uint32_t id_point;
    uint32_t id_cluster = -1;
    std::vector<uint32_t> values;
    uint32_t total_values;
    uint32_t pixel_count;

public:
    Point( uint32_t id_point, const std::vector<uint32_t>& values, uint32_t pixel_count );

    uint32_t getID() const;
    void setCluster( uint32_t id_cluster );
    uint32_t getCluster() const;
    uint32_t getPixelCount()const;

    const std::vector<uint32_t>& getValues() const;
};

class Cluster
{
private:
    uint32_t id_cluster;
    std::vector<double> central_values;
    std::vector<Point> points;

public:
    Cluster( uint32_t id_cluster, Point point );

    void addPoint( Point point );
    bool removePoint( uint32_t id_point );
    Point getPoint( uint32_t index ) const;
    uint32_t getTotalPoints() const;
    uint32_t getSize() const;

    const std::vector<double>& getCentralValues() const;
    std::vector<double>& getCentralValues();
};

class KMeans
{
private:
    uint32_t K; // number of clusters
    uint32_t colour_components, total_points, max_iterations;
    std::vector<Cluster> clusters;

    // return ID of nearest center
    // uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
    uint32_t getIDNearestCenter( Point point ) const;

public:
    KMeans( uint32_t K, uint32_t total_points, uint32_t  max_iterations );

    std::vector<Cluster> run( std::vector<Point> & points );
};

}