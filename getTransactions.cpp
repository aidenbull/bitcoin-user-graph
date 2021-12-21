/*
 * USAGE: ./getTransactions <start_block_index> <end_block_index> <filename> <chunksize=2>
 *
 * Parses the blockchain from <start_block_index> inclusive to <end_block_index> inclusive, and stores all transactions it finds in
 * json form in a file called "transactions-<filename>.txt". If such a file already exists, it appends to the existing file.
 * Additionally, whenever it stores transactions, it writes which block it has stored up to in a file called 
 * "transactionStoreLog-<filename>.txt". This is so in case some interruption occurs during collection, such as a power failure, 
 * you can continue collecting from that point. If this does occur, note that you will need to update <start_block_index>
 * according to the output in the log.
 *
 * <chunksize> is an optional argument that defaults to 2 which indicates how many blocks will be requested at each step. 
 * This option is unlikely to impact performance in any meaningful way, and is mostly valuable for reducing command line output.
 */


#include <iostream>
#include <vector>
#include <set>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "userGraph.hpp"
#include "structs.hpp"
#include <fstream>
#include <unordered_map>
#include <deque>

using json = nlohmann::json;
using namespace std;

//Added to store transaction outputs as they are read from Bitcoin Core. Typically the program is heavily bottlenecked by
// RPCs. The cache helps reduce the amount of RPCs significantly when obtaining transaction inputs.
class SimpleCache
{
    private:
        //The key stored in the map is the transaction id, with the index of the desired transaction output appended to the end
        unordered_map<string, txOutput> _map;
        //Needed a fifo queue to remove old transactions, would be nice to be able to delete items from the queue as they're used in order to
        // reduce queue bloat, but I couldn't think of a solution to removing arbitrary objects while maintaining temporal order
        deque<string> _fifo_queue;
        size_t _max_size;
        size_t _clear_amount;
        size_t _max_queue_size;
        size_t _queue_clear_amount;

        //Upon the map reaching _max_size, this function is called. It deletes elements popped from _fifo_queue until there are _max_size - _clear_amount
        // elements remaining
        void FreeCache()
        {
            cout << "Freeing Cache Space..." << endl;
            while(_map.size() > _max_size - _clear_amount)
            {
                _map.erase(_fifo_queue.front());
                _fifo_queue.pop_front();
            }
        }

        //Clears the fifo_queue if it grows too large, this usually occurs with high cache hit rate. Reduces queue size to _max_queue_size - _queue_clear_amount
        void FreeQueue()
        {
            cout << "Freeing Queue Space..." << endl;
            while(_fifo_queue.size() > _max_queue_size - _queue_clear_amount)
            {
                _map.erase(_fifo_queue.front());
                _fifo_queue.pop_front();
            }
        }

    public:
        SimpleCache(){}

        void Init(int max_size, int clear_amount, int max_queue_size, int queue_clear_amount)
        {
            _max_size = max_size;
            _clear_amount = clear_amount;
            _max_queue_size = max_queue_size;
            _queue_clear_amount = queue_clear_amount;
        }

        void AddElement(string key, txOutput val)
        {
            _map.insert({key, val});
            _fifo_queue.push_back(key);

            if (_map.size() > _max_size)
            {
                FreeCache();
            }
            else if (_fifo_queue.size() > _max_queue_size)
            {
                FreeQueue();
            }
        }

        bool Contains(string key)
        {
            return _map.count(key);
        }

        txInput Find(string key)
        {
            txOutput output = _map[key];
            return (txInput){.address=output.address, .value=output.value};
        }

        txInput FindAndRemove(string key)
        {
            txInput input = Find(key);
            _map.erase(key);
            return input;
        }

        int GetSize()
        {
            return _map.size();
        }
};

//Some global variables (spooky). Didn't want to pass the cache around as reference, as it would really bloat function calls.
// I make a similar argument for cache miss and hit rates. 
SimpleCache TxCache;
int cacheMisses = 0;
int cacheHits = 0;

//Formats a json rpc given a method and parameters. Because each parameter may or may not require quotes in the json rpc, i decided to leave them
// as a single string
string FormatRPC(string method, string params)
{
    return "{\"jsonrpc\": \"1.0\", \"id\": \"curltest\", \"method\": \"" + method + "\", \"params\": " + params + "}";
}

//Receives data from an RPC and appends the response to the userpointer string
size_t WriteCallback(char *content, size_t size, size_t nmemb, void *userpointer)
{
    string* responsePointer = (string*)userpointer;
    *responsePointer += content;
    //return the size of the output so curl knows you received the correct amount of data
    return size * nmemb;
}

//Perform an RPC and store the output in userPointer
void PerformRPC(string rpc, string* userPointer)
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, "http://aiden:5eiOEbJkAwiYx2gpWOag7YF5Lag@127.0.0.1:8332/");
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        //Assign our RPC to the post fields (don't forget to use a c string, not a c++ string (oops))
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, rpc.c_str());
        //Assign callback to handle received data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        //Assign our own pointer so we can store the data received by the writeCallback 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, userPointer);
        //Set argument to 1L for debug info
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            cout << "CURL ERROR -> " << string("curl_easy_perform() returned ") + curl_easy_strerror(res) + "\n";
        }

        curl_easy_cleanup(curl);
    }
}

//Perform getblockhash for range between low inclusive and high exclusive. Returns the value from the "result" field, or throws an exception if
// the "error" field is not null
vector<string> GetBlockHashRange(int low, int high)
{
    vector<string> hashes(high-low);
    string response;
    string params;
    json responseJSON;
    for (int i=low; i<high; i++)
    {
        response = "";
        params = "[" + to_string(i) + "]";
        string rpc = FormatRPC("getblockhash", params);
        PerformRPC(rpc, &response);

        responseJSON = json::parse(response);

        if (!responseJSON["error"].is_null()) throw std::runtime_error("bitcoind response error: " + to_string(responseJSON["error"]));

        hashes[i-low] = responseJSON["result"];
    }
    return hashes;
}

//Obtains the blocks with height between low inclusive and high exclusive. Skips many rpc steps by calling 
// getblock with verbosity 2, thus outputting all transactions directly. Returns a vector of json objects corresponding to
// the transactions stored in the blocks
vector<json> GetBlockRangeTransactions(int low, int high)
{
    //Get block hashes for the range
    vector<string> hashes = GetBlockHashRange(low, high);

    vector<json> txs;
    string response;
    string params;
    json responseJSON;
    for (int i=0; i<high-low; i++)
    {
        response = "";
        params = "[\"" + hashes[i] + "\",2]";
        string rpc = FormatRPC("getblock", params);
        PerformRPC(rpc, &response);

        responseJSON = json::parse(response);

        if (!responseJSON["error"].is_null()) throw std::runtime_error("bitcoind response error: " + to_string(responseJSON["error"]));

        for(json resultTxs : responseJSON["result"]["tx"])
        {
            txs.push_back(resultTxs);
        }
    }
    return txs;
}

//Uses option true to skip a second query to decoderawtransaction. Obtains a transaction directly. This function is
// called in the case of a cache miss, and will typically take up the majority of runtime.
json GetRawTransactionDirect(string txHash)
{
    string params = "[\"" + txHash + "\",true]";
    string rpc = FormatRPC("getrawtransaction", params);
    string response;

    PerformRPC(rpc, &response);

    json responseJSON = json::parse(response);

    return responseJSON["result"];
}

//Included because its somewhat non-trivial and is used in both GetTransactionInputs and GetTransactionOutputs. 
// Takes a single vOut json from a transaction as input and returns an address, which is either just a public key in
// the case of pay to public key (P2PK) transactions, or what's contained in the address field in case of pay to script hash (P2SH) 
// or pay to public key hash (P2PKH)
string GetAddressFromVOut(json vOut)
{
    string address;
    if (vOut["scriptPubKey"]["type"] == "pubkey")
    {
        //For pubkey transaction outputs, the asm field begins with the public key which is used to generate the bitcoin address
        // Converting a public key into a bitcon address is not a very simple task and I am omitting it for the moment. This will
        // generate false negatives in the user graph
        string asmString = vOut["scriptPubKey"]["asm"];
        size_t delimIndex = asmString.find(" ");
        address = asmString.substr(0, delimIndex);
    }
    else
    {
        //Addresses being in an array seems to indicate that a transaction output can go to multiple addresses which I do not understand
        // I am ignoring this issue for the moment. If i find a reason, i may need to adjust the hardcoded "0"
        address = vOut["scriptPubKey"]["addresses"][0];
    }
    return address;
}

//Takes a transaction json from Bitcoin Core and gathers the addresses and values of each input. Only gathers from P2PK, P2SH, and P2PKH
// transactions, as well as any other transaction that fills the address field
vector<txInput> GetTransactionInputs(json tx)
{
    vector<txInput> inputs;

    //In case this transaction is a coinbase transacton
    if (tx["vin"][0].contains("coinbase"))
    {
        inputs.push_back((txInput){.address = "coinbase", .value = tx["vout"][0]["value"]});
    }
    else
    {
        for (auto inTx : tx["vin"])
        {
            txInput input;

            string cacheKey = inTx["txid"];
            cacheKey += to_string(inTx["vout"]);
            if(TxCache.Contains(cacheKey)){
                //The transaction already exists in cache! Just read from there. Note that we remove from the cache
                // when we read an item, as a transaction output cannot be redeemed more than once
                input = TxCache.FindAndRemove(cacheKey);
                cacheHits++;
            }
            else
            {
                //The transaction does not exist in cache, so we have to request it from Bitcoin Core 
                int vOutIndex = inTx["vout"];

                json inTxJSON = GetRawTransactionDirect(inTx["txid"]);

                string address;
                //see transaction e411dbebd2f7d64dafeef9b14b5c59ec60c36779d43f850e5e347abee1e1a455 for details
                try
                {
                    address = GetAddressFromVOut(inTxJSON["vout"][vOutIndex]);
                }
                catch (json::type_error& e)
                {
                    continue;
                }

                float value = inTxJSON["vout"][vOutIndex]["value"];

                input = {.address=address, .value=value};
                cacheMisses++;
            }

            inputs.push_back(input);
        }
    }

    return inputs;
}

//Similar to GetTransactionInputs but for outputs. One notable difference is that instead of reading items from the cache,
// we store to the cache whenever we receive an item here
vector<txOutput> GetTransactionOutputs(json tx)
{
    vector<txOutput> outputs;
    
    for (auto vOut : tx["vout"])
    {
        //If this transaction output doesn't have value, ignore it
        if (vOut["value"] == 0) continue;

        string address;
        //see transaction e411dbebd2f7d64dafeef9b14b5c59ec60c36779d43f850e5e347abee1e1a455 for details
        try
        {
            address = GetAddressFromVOut(vOut);;
        }
        catch (json::type_error& e)
        {
            continue;
        }

        float value = vOut["value"];

        txOutput output = {.address = address, .value = value};

        //Add element to cache to hopefully avoid requesting an input from the server. Simply concatenating the transaction id with the vout index for the key
        string cacheKey = tx["txid"];
        TxCache.AddElement(cacheKey + to_string(vOut["n"]), output);

        outputs.push_back(output);
    }

    return outputs;
}

//Given transaction json from Bitcoin Core, creates a transaction struct storing only the addresses and the values of each input and output
transaction GetTransactionsFromJSON(json txJSON)
{
    transaction tx;

    tx.outputs = GetTransactionOutputs(txJSON);

    tx.inputs = GetTransactionInputs(txJSON);

    return tx;
}

//Given a vector of Bitcoin Core json transactions, converts each transaction into a transaction struct format
vector<transaction> GetTransactionsFromJSONVector(vector<json> txJSONs)
{
    vector<transaction> txs;

    for (json txJSON : txJSONs)
    {
        txs.push_back(GetTransactionsFromJSON(txJSON));
    }

    return txs;
}

//Prints out the elements of a string vector in order, separated by delim
void PrintStringVector(vector<string> vec, string delim="\n")
{
    for(size_t i=0; i<vec.size(); i++)
    {
        if (i>0) cout << delim;
        cout << vec[i];// << delim;
    }
    cout << endl;
}

//Prints out the elements of a string set, separated by delim
void PrintStringSet(set<string> stringSet, string delim="\n")
{
    for(string elem : stringSet)
    {
        //if (i>0) cout << delim;
        cout << elem << delim;
    }
    cout << endl;
}

//Prints a transaction struct. The output is verbose and includes line breaks so this may not be useful outside of debugging
void PrintTransactionStruct(transaction tx)
{
    cout << "INPUTS: " << endl;
    for (size_t i=0; i<tx.inputs.size(); i++)
    {
        cout << "  " + tx.inputs[i].address + ": " + to_string(tx.inputs[i].value) << endl;
    }

    cout << "OUTPUTS: " << endl;
    for (size_t i=0; i<tx.outputs.size(); i++)
    {
        cout << "  " + tx.outputs[i].address + ": " + to_string(tx.outputs[i].value) << endl;
    }
    cout << endl;
}

//Formats a transaction in a (relatively lightweight) json format to be written to file. The json output is of the form:
// {"inputs":[["<address>",<value>],...],"outputs":[["<address>",<value>],...]}
string ConvertTransactionToJSONString(transaction tx)
{
    string jsonString = "{\"inputs\":[";
    for(size_t i=0; i<tx.inputs.size(); i++)
    {
        txInput input = tx.inputs[i];
        jsonString = jsonString + "[\"" + input.address + "\"," + to_string(input.value) + "]";
        if (i+1<tx.inputs.size()) jsonString += ",";
    } 
    jsonString += "],\"outputs\":[";
    for(size_t i=0; i<tx.outputs.size(); i++)
    {
        txOutput output = tx.outputs[i];
        jsonString = jsonString + "[\"" + output.address + "\"," + to_string(output.value) + "]";
        if (i+1<tx.outputs.size()) jsonString += ",";
    } 
    jsonString += "]}";

    return jsonString;
}

//Outputs transactions stored in txs to the transactions file
void AppendTransactionsToFile(vector<transaction> txs, string filename)
{
    ofstream of(filename, ofstream::app);

    string outputBuffer;
    for(transaction tx : txs)
    {
        string jsonTx = ConvertTransactionToJSONString(tx);
        outputBuffer += jsonTx + "\n";
    }

    of << outputBuffer;

    //Close at each batch in case of interrupt
    of.close();
}

//Collects all transactions from those block indices startBlock inclusive and endBlock exclusive, reformats them into 
// our json format (see ConvertTransactionToJSONString) and stores them in the transactions file
void ObtainAndStoreTransactions(int startBlock, int endBlock, string filename)
{
    vector<json> blockTransactions = GetBlockRangeTransactions(startBlock, endBlock);

    vector<transaction> txs = GetTransactionsFromJSONVector(blockTransactions);
    
    AppendTransactionsToFile(txs, filename);
}

int main(int argc, char **argv)
{
    curl_global_init(CURL_GLOBAL_ALL);

    //Expecting format getTransactions <start_block_index> <end_block_index> <filename>
    if (argc < 4)
    {
        cout << "Error, expected format getTransactions <start_block_index> <end_block_index> <filename> <chunk_size=2>" << endl;
        return -1;
    }

    int startIndex;
    try
    {
        //using stoi instead of atoi to avoid undefined behaviour
        startIndex = stoi(string(argv[1]));
    }
    catch (const std::invalid_argument& ia)
    {
        cout << "Error, argument 1 is not an integer" << endl;
        return -1;
    }

    int endIndex;
    try
    {
        //using stoi instead of atoi to avoid undefined behaviour
        endIndex = stoi(string(argv[2]));
    }
    catch (const std::invalid_argument& ia)
    {
        cout << "Error, argument 2 is not an integer" << endl;
        return -1;
    }

    string filename = argv[3];

    int chunkSize = 2;
    if(argc > 4)
    {
        try
        {
            //using stoi instead of atoi to avoid undefined behaviour
            chunkSize = stoi(string(argv[4]));
        }
        catch (const std::invalid_argument& ia)
        {
            cout << "Error, argument 4 is not an integer" << endl;
            return -1;
        }
    }

    //You may need to update these values depending on how much memory you have available. The queue size in particular is a good place to
    // cut back if you're experiencing high memory usage.
    int CACHE_SIZE = 10000000;
    int CACHE_CLEAR_SIZE = 2000000;
    int FIFO_QUEUE_SIZE = 50000000;
    int FIFO_CLEAR_SIZE = 10000000;
    TxCache.Init(CACHE_SIZE, CACHE_CLEAR_SIZE, FIFO_QUEUE_SIZE, FIFO_CLEAR_SIZE);

    for(int i=startIndex; i<=endIndex; i+=chunkSize)
    {
        cacheHits = 0;
        cacheMisses = 0;

        //Just in case we're on the last few blocks so we don't include extra
        int truncatedEndIndex = min(i+chunkSize, endIndex+1);

        ObtainAndStoreTransactions(i, truncatedEndIndex, "outputs/transactions-" + filename + ".txt");

        cout << "Stored up to (but not including) block : " << to_string(truncatedEndIndex) << endl;
        cout << "cacheHits: " + to_string(cacheHits) << endl;
        cout << "cacheMisses: " + to_string(cacheMisses) << endl;
        cout << "cacheSize: " + to_string(TxCache.GetSize()) << endl;

        ofstream of("outputs/transactionStoreLog-" + filename + ".txt", ofstream::app);
        of << "Stored up to (but not including) block : " << to_string(truncatedEndIndex) << endl;
        of.close();
    }

    curl_global_cleanup();
    return 0;
}