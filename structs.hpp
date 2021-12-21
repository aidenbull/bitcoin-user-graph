#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <vector>

//Yeah these are redundant, couldnt think a good word to use to indicate either an input or output
struct txInput 
{
    std::string address;
    float value;
};

struct txOutput 
{
    std::string address;
    float value;
};

struct transaction 
{
    std::vector<txInput> inputs;
    std::vector<txOutput> outputs; 
} ;

//Created to make the program more memory efficient by only storing indices to an address vector instead of storing addresses multiple times
struct lightTxInput 
{
    int address;
    float value;
};

struct lightTxOutput 
{
    int address;
    float value;
};

struct lightTransaction
{
    std::vector<lightTxInput> inputs;
    std::vector<lightTxOutput> outputs;
} ;

#endif