# Simple Bitcoin User Graph, Aiden Bull

<h2>About</h2>

Simple c++ implementation for obtaining transaction data Bitcoin Core and using it to build a Bitcoin user graph. 

Created for my CSC499 honours project at UVic. The goal of this project was to compute a user graph using modern transaction data and then compare the results to those found in a study by Maesa et al. from 2017 [1]. The algorithm used to compute the user graph is also based off the one described in that paper.

<h2>Requirements</h2>

Requires c++17 to be installed on the system. 

Obtaining transaction info requires a full index Bitcoin Core node to be present and running on the computer. At the time of writing, a full index node consumes around 500GB of disk space. Obtaining transaction info also requires the libcurl library to be installed. Computing the user graph does not require Bitcoin Core or libcurl.

Makes use of <a href="https://github.com/nlohmann/json">nlohmann's JSON library</a>. This library contained a bug with c++17 at the time of writing which caused the program to fail, so a simple workaround was added. The modified library is included in the `include/` directory.

Some simple graph statistics were also obtained using R with the igraph library. This script is included in the `r/` directory.

<h2>Usage</h2>

Running `make` will generate two binary files: `getTransactions` and `calculateUserGraph`. These files contain comments at the beginning of the file describing usage and output in detail. Please refer to these comments for detailed info.

`getTransactions` is the program that obtains transaction info. As mentioned in the requirements section, it requires Bitcoin Core to be installed and running. Simple usage for `getTransactions` is as follows: 
    `getTransactions <start_block_index> <end_block_index> <filename>`
This will produce two files in the `output/` directory: `transactions-<filename>.txt` and `transactionStoreLog-<filename>.txt`. The first file contains the transaction info required for producing the user graph. The second file contains info that will be useful for resuming where you left off if `getTransactions` is interrupted for any reason.

`calculateUserGraph` is the program that computes a user graph using transaction info obtained from `getTransactions`. Usage for `calculateUserGraph` is as follows:
    `getTransactions <filename>`
The `<filename>`  used here should be identical to the one used when executing `getTransactions`. This program will read transaction info from the `transactions-<filename>.txt` file, and use it to produce 2 files: `usergraph-<filename>.txt` and `stats-<filename>.txt`. The first file contains an edge list for the user graph obtained from the input transactions. The second file contains some simple statistics about the resulting user graph.

<h2>Thanks</h2>

I want to thank Professor Alex Thomo for helping and guiding me throughout the term, this project would not have been completed without his help.

<h2>References</h2>

[1] - Di Francesco Maesa, D., Marino, A. & Ricci, L. Data-driven analysis of Bitcoin properties: exploiting the users graph. Int J Data Sci Anal 6, 63–80 (2018). https://doi-org.ezproxy.library.uvic.ca/10.1007/s41060-017-0074-x 

<h2>License</h2>

Feel free to use and distribute my code as you see fit if it seems that it will be helpful to you. If you have any questions about the code feel free to post an issue on the github repo or to contact me at `aidenbull@hotmail.com`


The following license is copied from the license section of nlohmann's json github repository. It is included here because the library was used in this project.


The class is licensed under the MIT License:

Copyright © 2013-2021 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

