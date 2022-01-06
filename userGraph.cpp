/* 
 * Contains graph and usergraph implementation to be called by calculateUserGraph.cpp 
 */

#include "structs.hpp"
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <stack>

using namespace std;

//Just a simple graph implementation along with DFS to compute connected components. 
// Only supports integer ids for nodes to keep memory consumption to a minimum, therefore a mapping should be created
// externally to relate node ids to objects
class Graph
{
    private:
        int _numVertices;
        //Adjacency list, where an integer j contained in the set at index i refers to an edge from i to j. The set at index i always contains i
        vector<unordered_set<int>> _adjList;

        vector<bool> _visited;
        vector<int> DFS(int v)
        {
            vector<int> component;
            stack<int> dfsStack;

            dfsStack.push(v);
            _visited[v] = true;

            while(!dfsStack.empty())
            {
                int curr = dfsStack.top();
                dfsStack.pop();
                component.push_back(curr);

                for (int adjacentVertex : _adjList[curr])
                {
                    if (!_visited[adjacentVertex]) 
                    {
                        dfsStack.push(adjacentVertex); 
                        _visited[adjacentVertex] = true;
                    }
                }
            }

            return component;
        }

    public:
        //Constructor only takes one parameter, which is the amount of nodes in the graph. This graph does not support insertion
        // and removal of nodes after instantiation. Each node is simply referred to by index between [0, numVertices)
        Graph(int numVertices)
        {
            _numVertices = numVertices;

            for (int i=0; i<_numVertices; i++)
            {
                unordered_set<int> temp; 
                _adjList.push_back(temp);
                _adjList[i].insert(i);
            }
        }

        void AddEdge(int v1, int v2)
        {
            //Doesn't allow self loops, remove this check if you would like to include them
            if (v1 != v2)
                _adjList[v1].insert(v2);
        }

        void AddUndirectedEdge(int v1, int v2)
        {
            AddEdge(v1, v2);
            AddEdge(v2, v1);
        }

        //Calculates the connected components in the graph. Returns the components, stored as a vector of vectors of ints as well. 
        // Each internal vector refers to one connected component, with the integers of that vector corresponding to the nodes belonging 
        // to that component. Also returns a vector of ints, which maps nodes to their component ids. Each index of this vector corresponds
        // to a node, and the value at that index corresponds to a component index.
        pair<vector<vector<int>>, vector<int>> CalculateConnectedComponents()
        {
            //For each cluster, contains a list of all address indices in that cluster
            vector<vector<int>> components;
            //Maps address vector indices to cluster indices
            vector<int> addressMap;
            //Assigning -1 to make bugs more obvious hopefully
            addressMap.assign(_numVertices, -1);

            int componentCount = 0;
            
            //Doing this outside of DFS because i dont want to perform an O(n) for each cluster
            _visited.assign(_numVertices, false);

            for (int i=0; i<_numVertices; i++)
            {
                if (_visited[i]) continue;

                vector<int> currComponent = DFS(i);
                for (int currVertex : currComponent)
                {
                    addressMap[currVertex] = componentCount;
                }
                components.push_back(currComponent);

                componentCount++;
            }

            return pair<vector<vector<int>>, vector<int>>(components, addressMap);
        }
};

//The actual user graph class. Takes a list of clusters as input, along with a mapping of addresses (in this case integer ids to save memory) 
// to clusters, and a list of transactions. 
class UserGraph
{
    private:
        vector<vector<int>> _clusters;
        vector<int> _clusterMap;
        vector<unordered_map<int, float>> _weightedAdjList;
        //Just included because the paper that I wanted to compare data to had parallel edges for some statistics
	    vector<vector<pair<int, float>>> _multiGraphWeightedAdjList;

        //Updates _weightedAdjList
        void AddOrUpdateWeightedEdge(int v1, int v2, float value)
        {
            if (v1 != v2)
            {
                float valueToAssign = value;
                if (_weightedAdjList[v1].count(v2))
                    valueToAssign += _weightedAdjList[v1][v2];
                _weightedAdjList[v1].insert_or_assign(v2, valueToAssign);
            }
        }

        //Updates _multiGraphWeightedAdjList
        void AddWeightedEdge(int v1, int v2, float value)
        {
            _multiGraphWeightedAdjList[v1].push_back({v2, value});
        }

    public:
        UserGraph(vector<vector<int>>* clusters, vector<int> *clusterMap, vector<lightTransaction> txs, bool multiGraph=true)
            : _clusters{*clusters},
              _clusterMap{*clusterMap},
              _weightedAdjList{(*clusters).size()},
              _multiGraphWeightedAdjList{(*clusters).size()}
        { 
            for (lightTransaction tx : txs)
            {   
                //In case our code wasn't able to to find the address for any of the inputs for a given transaction
                if (tx.inputs.size() == 0) continue;

                int inputCluster = _clusterMap[tx.inputs[0].address];
                for (lightTxOutput output : tx.outputs)
                {
                    int outputCluster = _clusterMap[output.address];
                    if (!multiGraph)
                    {
                        AddOrUpdateWeightedEdge(inputCluster, outputCluster, output.value);
                    }
                    else
                    {
                        AddWeightedEdge(inputCluster, outputCluster, output.value);
                    }
                }
            }
        }

        vector<vector<int>> GetClusters()
        {
            return _clusters;
        }

        vector<int> GetClusterMap()
        {
            return _clusterMap;
        }

        //Need to convert unordered maps to vector<pair<int, float>> to keep consistent with multi graph output
        vector<vector<pair<int, float>>> GetEdges()
        {
            vector<vector<pair<int, float>>> adjList;
            for(unordered_map<int, float> map : _weightedAdjList)
            {
                vector<pair<int, float>> temp(map.begin(), map.end());
                adjList.push_back(temp);
            }
            return adjList;
        }

        vector<vector<pair<int, float>>> GetMultiGraphEdges()
        {	
            return _multiGraphWeightedAdjList;
        }
};

//Creates the user graph given a vector of clusters, a map from address to cluster, and a vector of transactions
UserGraph CreateUserGraph(vector<vector<int>>* clusters, vector<int>* clusterMap, vector<lightTransaction> txs, multiGraph=true)
{
    UserGraph userGraph(clusters, clusterMap, txs, multiGraph);
    return userGraph;
}

//Calls CreateUserGraph and returns the edges from the resulting graph
vector<vector<pair<int, float>>> CreateAndDumpUserGraph(vector<vector<int>>* clusters, vector<int>* clusterMap, vector<lightTransaction> txs)
{   
    UserGraph userGraph = CreateUserGraph(clusters, clusterMap, txs);
    
    return userGraph.GetEdges();
}

//Computes address clusters using the multiple inputs heuristic. Takes a vector of addresses as well as a vector containing
// all transactions, and clusters addresses together if they are both used as input to the same transaction. See Graph.CalculateConnectedComponents()
// for explanation of return values
pair<vector<vector<int>>, vector<int>> FindClusters(vector<string> addresses, vector<lightTransaction> txs)
{
    Graph clusterGraph(addresses.size());

    for (lightTransaction tx : txs)
    {
        for (size_t j=0; j<tx.inputs.size()-1; j++)
        {  
            //In case our code wasn't able to to find the address for any of the inputs for a given transaction
            if (tx.inputs.size() == 0) break;
            clusterGraph.AddUndirectedEdge(tx.inputs[j].address, tx.inputs[j+1].address);
        }
    }

    return clusterGraph.CalculateConnectedComponents();
}
