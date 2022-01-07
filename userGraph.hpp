#ifndef USERGRAPH_H
#define USERGRAPH_H

#include "structs.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Graph
{
    public:
        Graph(int numVertices);

        void AddEdge(int v1, int v2);

        void AddUndirectedEdge(int v1, int v2);

        std::pair<std::vector<std::vector<int>>, std::vector<int>> CalculateConnectedComponents();
};

class UserGraph
{
    public:
        UserGraph(std::vector<std::vector<int>>* clusters, std::vector<int>* clusterMap, std::vector<lightTransaction> txs);

        std::vector<std::vector<int>> GetClusters();

        std::vector<int> GetClusterMap();

        std::vector<std::vector<std::pair<int, float>>> GetEdges();
};

std::vector<std::vector<std::pair<int, float>>> CreateUserGraph(std::vector<std::vector<int>>* clusters, std::vector<int>* clusterMap, std::vector<lightTransaction> txs, bool isMultiGraph=true);

std::pair<std::vector<std::vector<int>>, std::vector<int>> FindClusters(std::vector<std::string> addresses, std::vector<lightTransaction> txs);

#endif