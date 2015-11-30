#pragma once
#include "RedundancyEliminator_CPP.h"
#include "LRUHash.h"

class RedundancyEliminator_CPP_CollisionTest: public RedundancyEliminator_CPP
{
private:
	LRUHash<unsigned long long> rabinLRUHash;
	LRUStrHash<SHA_DIGEST_LENGTH> sha1LRUHash;
	LRUStrHash<MD5_DIGEST_LENGTH> md5LRUHash;

	unordered_map<unsigned long long, 
				  tuple<char*, int>> 
				  rabinMap;
	unordered_map<unsigned char*, 
				  tuple<char*, int>, 
				  CharArrayHashFunc<SHA_DIGEST_LENGTH>, 
				  CharArrayEqualTo<SHA_DIGEST_LENGTH>> 
				  sha1Map;
	unordered_map<unsigned char*,
				  tuple<char*, int>,
				  CharArrayHashFunc<MD5_DIGEST_LENGTH>,
				  CharArrayEqualTo<MD5_DIGEST_LENGTH>>
				  md5Map;

	inline void addNewChunkRabin(unsigned long long hashValue, 
								 char* chunk, 
								 unsigned int chunkSize, 
								 bool isDuplicate);

	inline void addNewChunkSha1(unsigned char* hashValue,
								char* chunk,
								unsigned int chunkSize,
								bool isDuplicate);

	inline void addNewChunkMd5(unsigned char* hashValue,
							   char* chunk,
							   unsigned int chunkSize,
							   bool isDuplicate);
public:
	RedundancyEliminator_CPP_CollisionTest();
	void SetupRedundancyEliminator_CPP_CollisionTest();
	~RedundancyEliminator_CPP_CollisionTest();

	inline unsigned long long ComputeRabinHash(char* chunk, unsigned int chunkSize);
	unsigned int RabinFingerPrinting(deque<unsigned int> indexQ, char* package);
	unsigned int Sha1FingerPrinting(deque<unsigned int> indexQ, char* package);
	unsigned int Md5FingerPrinting(deque<unsigned int> indexQ, char* package);
	tuple<unsigned int, unsigned int> 
		RabinFingerPrintingWithCollisionCheck(deque<unsigned int> 
											  indexQ, char* package);
	tuple<unsigned int, unsigned int>
		Sha1FingerPrintingWithCollisionCheck(deque<unsigned int> indexQ,
											 char* package);
	tuple<unsigned int, unsigned int>
		Md5FingerPrintingWithCollisionCheck(deque<unsigned int> indexQ,
											char* package);
};