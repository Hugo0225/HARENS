#pragma once
#include "RabinHash.h"
#include "LRUHash.h"
#include "Definition.h"
class RedundancyEliminator_CPP {
protected:
	RabinHash hashFunc;
	LRUHash circHash;

	//Add a new chunk into cache, if hash value queue is full also delete the oldest chunk
	void addNewChunk(unsigned char* hashValue, char* chunk
		, unsigned int chunkSize, bool isDuplicate);
	inline void computeChunkHash(char* chunk, unsigned int chunkSize, unsigned char *hashValue);
		
public:
	RedundancyEliminator_CPP();
	void SetupRedundancyEliminator_CPP();
	~RedundancyEliminator_CPP();

	deque<unsigned int> chunking(char* package, unsigned int packageSize);
	unsigned int fingerPrinting(deque<unsigned int> indexQ, char* package);
	unsigned int eliminateRedundancy(char* package, unsigned int packageSize);
};