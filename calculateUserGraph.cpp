/*
 * USAGE: ./calculateUserGraph <filename>
 *
 * First collects transaction info as output by getTransactions.cpp, uses the transaction info to compute a user graph,
 * and stores the user graph as an edge list along with various basic statistics of the graph. <filename> should be same as what
 * was passed to getTransactions.cpp. For example, after running getTransactions, the program outputs a file called 
 * "transactions-<filename>.txt", passing <filename> to this program will cause it to read from that file. Produces two files as
 * output. One is "userGraph-<filename>.txt" which contains the usergraph edge list, along with a prepended line containing column
 * names. The other is "stats-<filename>.txt" which contains some basic info about the graph.
 * 
 * This file, as of the time of writing, is fairly memory hungry. For example, an input file with size around 20GB can be expected to
 * consume around 50GB of memory. Some measures have been taken to make it less memory hungry, such as only storing one copy of each 
 * address and having all data structures refer to it by index, but there is likely still improvements to be made. The lightTransaction 
 * struct might be a good place to start. Using a smaller integer type for graph data structures may also lead to improvements. Another
 * option would be to discard data structures after computing the next step with them.
 */ 

#include <iostream>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "userGraph.hpp"
#include "structs.hpp"
#include <fstream>
#include <unordered_map>
#include <deque>

using json = nlohmann::json;
using namespace std;


//Given a file pointer containing a list of json-encoded transactions, returns a vector of addresses, as well as a vector of 
// all transactions, storing indices of the address vector instead of full addresses.
pair<vector<lightTransaction>, vector<string>> MemoryLightReadTransactionsFromFile(ifstream* is)
{
    vector<lightTransaction> txs;
    unordered_map<string, int> addressToVectorMap;
    vector<string> addressVector;

    string line;
    while(!getline(*is, line).eof())
    {
        json jsonTx = json::parse(line);

        lightTransaction tx;

        //The following might be the biggest arguments against having txInput and txOutput as separate structs.
        // I'm leaving it for now, but that's just due to me being low on time. If I have the time, I will try to clean 
        // this up. If it's here while you're using it, feel free to post an issue on the github asking me to fix it and 
        // I will try to get to it ASAP.

        //Handle inputs
        for(auto input : jsonTx["inputs"])
        {
            int index;
            string address = input[0];
            float value = input[1];
            if (addressToVectorMap.count(address))
            {
                //The address already exists in the vector, return the correct index
                index = addressToVectorMap[address];
            }
            else
            {
                //The address does not already exist in the vector, add the address to the vector and add the correct mapping
                index = addressVector.size();
                addressToVectorMap.insert({address, index});
                addressVector.push_back(address);
            }
            tx.inputs.push_back((lightTxInput){.address=index, .value=value});
        }

        //Handle outputs
        for(auto output : jsonTx["outputs"])
        {
            int index;
            string address = output[0];
            float value = output[1];
            if (addressToVectorMap.count(address))
            {
                //The address already exists in the vector, return the correct index
                index = addressToVectorMap[address];
            }
            else
            {
                //The address does not already exist in the vector, add the address to the vector and add the correct mapping
                index = addressVector.size();
                addressToVectorMap.insert({address, index});
                addressVector.push_back(address);
            }
            tx.outputs.push_back((lightTxOutput){.address=index, .value=value});
        }

        txs.push_back(tx);
    }

    return pair<vector<lightTransaction>, vector<string>>{txs, addressVector};
}

//Helper function for CalculateAndStoreLargestClusters. Given a list of clusters as well as a reference
// to a vector containing cluster ids, reorders the ids in decreasing order of cluster size. Takes advantage
// of the fact that largestClusters is already sorted except for the last item.
void ReorderLargestClusterList(vector<vector<int>> clusters, vector<int>* largestClusters)
{
    for (size_t i=(*largestClusters).size()-1; i>0; i--)
    {
        if (clusters[(*largestClusters)[i]].size() > clusters[(*largestClusters)[i-1]].size())
        {
            swap((*largestClusters)[i], (*largestClusters)[i-1]);
        }
        else
        {
            break;
        }
    }
}

//Calculates the 10 largest clusters and returns a vector storing their ids.
vector<int> CalculateAndStoreLargestClusters(vector<vector<int>> clusters)
{
    size_t maxLargestClusters = 10;
    vector<int> largestClusters;
    for (size_t i=0; i<clusters.size(); i++)
    {
        if (largestClusters.size() < maxLargestClusters)
        {
            largestClusters.push_back(i);
            ReorderLargestClusterList(clusters, &largestClusters);
        }
        else if (clusters[i].size() > clusters[largestClusters.back()].size())
        {
            largestClusters[maxLargestClusters - 1] = i;
            ReorderLargestClusterList(clusters, &largestClusters);
        }
    }
    return largestClusters;
}

//Calculates the value flowing into and out of every user graph cluster and returns a vector parallel to userGraphEdges
// containing these values
vector<pair<float, float>> CalculateClusterRichness(vector<unordered_map<int, float>> userGraphEdges)
{
    vector<pair<float, float>> clusterValues(userGraphEdges.size());
    for(size_t i=0; i<userGraphEdges.size(); i++)
    {
        unordered_map<int, float> payer = userGraphEdges[i];
        for(auto payee : payer)
        {
            int cluster = payee.first;
            float value = payee.second;
            clusterValues[i].second += value;
            clusterValues[cluster].first += value;
        }
    }
    return clusterValues;
}

//Simple helper function to reduce verbosity. Calculates cluster richness, being value in - value out of a cluster
float ClusterValue(pair<float, float> cluster)
{
    return cluster.first - cluster.second;
}

//Given a list of the richest clusters, reorders them according to cluster value as described above
void ReorderRichestClusterList(vector<pair<int, pair<float, float>>>* richestClusters)
{
    for (size_t i=(*richestClusters).size()-1; i>0; i--)
    {
        if (ClusterValue((*richestClusters)[i].second) > ClusterValue((*richestClusters)[i-1].second))
        {
            swap((*richestClusters)[i], (*richestClusters)[i-1]);
        }
        else
        {
            break;
        }
    }
}

//Calculates the 10 richest clusters according to amount of value going into the cluster minus the amount of value
// coming out of the cluster. Returns a vector storing first the cluster id, then a pair containing value in and value out
vector<pair<int, pair<float, float>>> CalculateRichestClusters(vector<unordered_map<int, float>> userGraphEdges)
{
    vector<pair<float, float>> clusterValues = CalculateClusterRichness(userGraphEdges);
    
    size_t maxRichestClusters = 10;
    vector<pair<int, pair<float, float>>> richestClusters;

    for (size_t i=0; i<clusterValues.size(); i++)
    {
        pair<int, pair<float, float>> clusterValueItem = {i, clusterValues[i]};
        if (richestClusters.size() < maxRichestClusters)
        {
            richestClusters.push_back(clusterValueItem);
            ReorderRichestClusterList(&richestClusters);
        }
        else if (ClusterValue(clusterValues[i]) > ClusterValue(richestClusters.back().second))
        {
            richestClusters[maxRichestClusters - 1] = clusterValueItem;
            ReorderRichestClusterList(&richestClusters);
        }
    }

    return richestClusters;
}

//Collects various basic statistics about a user graph and outputs the statistics to a file called "stats-<filename>.txt". Statistics included are:
// number of transactions, number of unique addresses, number of clusters, largest clusters by address count, number of user graph edges,
// and richest clusters according to value in - value out
void PrintStatsToFile(string filename, vector<lightTransaction> txs, vector<string> addresses, vector<vector<int>> clusters, vector<unordered_map<int, float>> userGraphEdges)
{
    ofstream os ("outputs/stats-" + filename + ".txt", ifstream::out);

    os << "Number of transactions: " << txs.size() << endl;

    os << "Number of unique addresses: " << addresses.size() << endl;

    os << "Number of clusters: " << clusters.size() << endl;

    vector<int> largestClusters = CalculateAndStoreLargestClusters(clusters);

    os << "Largest clusters and number of addresses: " << endl;

    for(int cluster : largestClusters)
    {
        os << "  " << cluster << ":" << clusters[cluster].size() << endl;
    }

    int numEdges = 0;

    for(unordered_map<int, float> cluster : userGraphEdges)
    {
        numEdges += cluster.size();
    }

    os << "Number of User Graph edges: " << numEdges << endl;

    vector<pair<int, pair<float, float>>> richestClusters = CalculateRichestClusters(userGraphEdges);

    os << "Richest clusters and input-output total: " << endl;

    for(auto cluster : richestClusters)
    {
        os << "  " << cluster.first << " " << cluster.second.first << " " << cluster.second.second << endl;
    }

    os.close();
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        cout << "Error, expected format calculateUserGraph <filename>" << endl;
        return -1;
    }

    string filename = argv[1];

    vector<string> addresses;

    vector<lightTransaction> lightTxs;

    cout << "Reading transactions from input... " << flush;
    string inputFileName = "outputs/transactions-" + filename + ".txt";
    ifstream is (inputFileName, ifstream::in);
    tie(lightTxs, addresses) = MemoryLightReadTransactionsFromFile(&is);
    is.close();
    cout << "Done" << endl;

    vector<vector<int>> clusters;
    vector<int> clusterMap;

    cout << "Calculating clusters... " << flush;
    tie(clusters, clusterMap) = FindClusters(addresses, lightTxs);
    cout << "Done" << endl;

    vector<unordered_map<int, float>> userGraphEdges;

    cout << "Calculating usergraph... " << flush;
    userGraphEdges = CreateAndDumpUserGraph(&clusters, &clusterMap, lightTxs);
    cout << "Done" << endl;

    cout << "Writing stats to file... " << flush;
    PrintStatsToFile(filename, lightTxs, addresses, clusters, userGraphEdges);
    cout << "Done" << endl;

    cout << "Writing usergraph to file... " << flush;
    ofstream os ("outputs/userGraph-" + filename + ".txt", ifstream::out);
    os << "from to weight" << endl;
    for(size_t i=0; i<userGraphEdges.size(); i++)
    {
        unordered_map<int, float> edges = userGraphEdges[i];
        for (auto const& [key, val] : edges)
        {
            os << i << " " << key << " " << val << endl;
        }
    }
    os.close();
    cout << "Done" << endl;

    return 0;
}
