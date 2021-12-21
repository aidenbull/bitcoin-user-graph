all : getTransactions userGraph

getTransactions : getTransactions.cpp
	g++ -std=c++17 -I ./include -Wall -O3 getTransactions.cpp -o getTransactions -lcurl

profUserGraph : calculateUserGraph.cpp userGraph.cpp
	g++ -std=c++17 -I ./include -Wall -O3 -pg calculateUserGraph.cpp userGraph.cpp -o calculateUserGraph

userGraph : calculateUserGraph.cpp userGraph.cpp
	g++ -std=c++17 -I ./include -Wall -O3 calculateUserGraph.cpp userGraph.cpp -o calculateUserGraph