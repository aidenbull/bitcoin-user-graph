# Simple Bitcoin User Graph, Aiden Bull

<h2>About</h2>

Simple c++ implementation for obtaining transaction data Bitcoin Core and using it to build a Bitcoin user graph. 

Created for my CSC499 honours project at UVic. The goal of this project was to compute a user graph using modern transaction data and then compare the results to those found in a study by Maesa et al. from 2017 [1]. The algorithm used to compute the user graph is also based off the one described in that paper.

<h2>Requirements</h2>

Requires c++17 to be installed on the system. 

Obtaining transaction info requires a full index Bitcoin Core node to be present and running on the computer. At the time of writing, a completely synced full index node consumes around 500GB of disk space. However, if you only want to analyze older data, your node will only need to be synced up to the block height of interest. Obtaining transaction info also requires the libcurl-dev package to be installed. Computing the user graph does not require Bitcoin Core or libcurl.

Makes use of <a href="https://github.com/nlohmann/json">nlohmann's JSON library</a>. This library contained a bug with c++17 at the time of writing which caused the program to fail, so a simple workaround was added. The modified library is included in the `include/` directory.

Some simple graph statistics were also obtained using R with the igraph library. This script is included in the `r/` directory.


<h3>PreReq installation guide</h3>

The following guide is designed for Debian-based systems. Some steps may be different for other operating systems.


Bitcoin Core:

Bitcoin Core can be downloaded <a href="https://bitcoin.org/en/version-history">here</a>. In order to install it, first choose which version you would like to use, and download the appropriate tar.gz for your computer architecture. This project was developed using Bitcoin Core 0.21.1, but any more recent version should work fine.

Once you've downloaded the package, unzip it, and then open a terminal inside the resulting directory. To install it, execute the following command:

`sudo install -m 0755 -o root -g root -t /usr/local/bin bin/*`

The command will copy the components from the local bin directory into /usr/local/bin, where they can be executed from anywhere, as well as set file permissions and ownership.

Once the files have been installed, run `bitcoind` for the first time. This will start up a bitcoin P2P node from your computer, and your computer will begin collecting block data from nearby peers. Running `bitcoind` for the first time will also generate a folder called `.bitcoin` in your home directory. Once this folder has been created, shut down bitcoin core using ctrl-c interrupt or by opening another terminal and executing the command `bitcoin-cli stop`. Now, you will need to create a config file for Bitcoin Core, the default config file that Bitcoin Core checks on startup is `.bitcoin/bitcoin.conf`. Bitcoin Core does not create this file automatically, and so you must do this yourself. An example config file can be seen <a href="https://github.com/bitcoin/bitcoin/blob/master/share/examples/bitcoin.conf">here</a>. For this project, you will need to set the following options: `txindex=1`, `server=1`, `rpcuser=<YOUR-USERNAME>`, and `rpcpassword=<YOUR-SECURE-PASSWORD>`. Once these options have been set, Bitcoin Core is now installed and set up. Now you can run `bitcoind` to let Bitcoin Core sync. 

WARNING! - Running a bitcoin node can consume up to 500GB of storage at the time of writing. It can also consume a large amount of network data quickly. If your internet setup is data-limited, running this program for an extended period of time can lead to charges. If at any point you want to use Bitcoin Core without acting as a P2P node, run `bitcoind`, and then in another terminal, execute `bitcoin-cli setnetworkactive false`. This will stop all peer to peer activity, but still allow you to use Bitcoin Core locally. This project does not require a fully synced node to work, but it does require the node to be synced up to the block height that you are interested in analyzing.


libcurl-dev:

Installing libcurl-dev can be done using apt install. libcurl-dev is a virtual package, and because of this, you will have to choose a specific implementation from a list. These options can be seen by executing the following command:

`sudo apt install libcurl-dev`

One of these options can then be selected by running the same apt install command, but with the name of the implementation you choose instead of 'libcurl-dev'. The project was developed using `libcurl4-openssl-dev`, but any of the options should work fine.


<h2>Usage</h2>

Running `make` will generate two binary files: `getTransactions` and `calculateUserGraph`. The files `getTransactions.cpp` and `calculateUserGraph.cpp` contain comments at the beginning describing usage and output in detail. Please refer to these comments for more detailed info.


`getTransactions` is the program that obtains transaction info. As mentioned in the requirements section, it requires Bitcoin Core to be installed and running. Simple usage for `getTransactions` is as follows: 

`getTransactions <start_block_index> <end_block_index> <filename>`

This will produce two files in the `output/` directory: `transactions-<filename>.txt` and `transactionStoreLog-<filename>.txt`. The first file contains the transaction info required for producing the user graph. The second file contains info that will be useful for resuming where you left off if `getTransactions` is interrupted for any reason. `getTransactions` also reads some input from `config.json`. The values in `config.json` are values that likely won't need to be changed between executions. Important values that need to be set are rpcuser and rpcpassword. These values are required for interfacing with Bitcoin Core, and should match the values stored in .bitcoin/bitcoin.conf. See <a href="https://github.com/bitcoin/bitcoin/blob/master/share/examples/bitcoin.conf">rpcuser and rpcpassword</a> for more info. 


`calculateUserGraph` is the program that computes a user graph using transaction info obtained from `getTransactions`. Usage for `calculateUserGraph` is as follows:

`getTransactions <filename>`

The `<filename>`  used here should be identical to the one used when executing `getTransactions`. This program will read transaction info from the `transactions-<filename>.txt` file, and use it to produce 2 files: `userGraph-<filename>.txt` and `stats-<filename>.txt`. The first file contains an edge list for the user graph obtained from the input transactions. The second file contains some simple statistics about the resulting user graph.

<h2>Thanks</h2>

I want to thank Professor Alex Thomo for helping and guiding me throughout the term, this project would not have been completed without his help.

<h2>Sample</h2>

A sample graph obtained from blocks 670001-700000 can be found <a href="https://drive.google.com/file/d/1dgRzjyYs-iBvQ_JzQK8175NqWLZMJGgV/view?usp=sharing">here</a> (2.6 GB). 

<h2>References</h2>

[1] - Di Francesco Maesa, D., Marino, A. & Ricci, L. Data-driven analysis of Bitcoin properties: exploiting the users graph. Int J Data Sci Anal 6, 63–80 (2018). https://doi-org.ezproxy.library.uvic.ca/10.1007/s41060-017-0074-x 

<h2>License</h2>

Feel free to use and distribute my code as you see fit if it seems helpful to you. If you have any questions about the code feel free to post an issue on the github repo or to contact me at `aidenbull@hotmail.com`

-

The following license is copied from the license section of nlohmann's json github repository. It is included here because the library was used in this project.


The class is licensed under the MIT License:

Copyright © 2013-2021 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

