#ifndef USERGRAPH_H
#define USERGRAPH_H

#include "structs.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Graph
{

    private:
        int _numVertices;
        std::vector<std::vector<int>> _adjList;

        std::vector<int> CalculateConnectedComponent(int v);

        std::vector<int> DFS(int v);

    public:
        Graph(int numVertices);

        void AddEdge(int v1, int v2);

        void AddUndirectedEdge(int v1, int v2);

        std::pair<std::vector<std::vector<int>>, std::vector<int>> CalculateConnectedComponents();
};

class UserGraph
{
    private:
        std::vector<std::vector<int>> _clusters;
        std::vector<int> _clusterMap;
        std::vector<std::unordered_map<int, float>> _weightedAdjList;
        
        void AddOrUpdateWeightedEdge(int v1, int v2, float value);

    public:
        UserGraph(std::vector<std::vector<int>>* clusters, std::vector<int>* clusterMap, std::vector<lightTransaction> txs);

        std::vector<std::vector<int>> GetClusters();

        std::vector<int> GetClusterMap();

        std::vector<std::unordered_map<int, float>> GetEdges();
};

UserGraph CreateUserGraph(std::vector<std::vector<int>>* clusters, std::vector<int>* clusterMap, std::vector<lightTransaction> txs);

std::vector<std::unordered_map<int, float>> CreateAndDumpUserGraph(std::vector<std::vector<int>>* clusters, std::vector<int>* clusterMap, std::vector<lightTransaction> txs);

std::pair<std::vector<std::vector<int>>, std::vector<int>> FindClusters(std::vector<std::string> addresses, std::vector<lightTransaction> txs);

#endif