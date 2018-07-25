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
    int id_point, id_cluster;
    std::vector<uint32_t> values;
    uint32_t total_values;
    uint32_t pixel_count;

public:
    Point( int id_point, const std::vector<uint32_t>& values, uint32_t pixel_count );

    int getID() const;
    void setCluster( int id_cluster );
    int getCluster() const;
    uint32_t getValue( int index ) const;
    uint32_t getTotalValues() const;
    uint32_t getPixelCount()const;
};

class Cluster
{
private:
    int id_cluster;
    std::vector<double> central_values;
    std::vector<Point> points;

public:
    Cluster( int id_cluster, Point point );

    void addPoint( Point point );
    bool removePoint( int id_point );
    double getCentralValue( int index ) const;
    void setCentralValue( int index, double value );
    Point getPoint( int index ) const;
    int getTotalPoints() const;
    int getSize() const;
};

class KMeans
{
private:
    int K; // number of clusters
    int colour_components, total_points, max_iterations;
    std::vector<Cluster> clusters;

    // return ID of nearest center
    // uses distance calculations from: https://en.wikipedia.org/wiki/Color_difference
    int getIDNearestCenter( Point point ) const;

public:
    KMeans( int K, int total_points, int max_iterations );

    std::vector<Cluster> run( std::vector<Point> & points );
};

}