#include "LRUHash.h"

template <int str_len>
LRUHash<str_len>::LRUHash<str_len>(unsigned int _size) : VirtualHash<str_len>(_size), circularQueue(_size), map(size)
{
}

template <int str_len>
void LRUHash<str_len>::SetupLRUHash(unsigned int _size) {
	SetupVirtualHash(_size);
	circularQueue.SetupLRUQueue(_size);
	map = charPtMap(_size);
}

template <int str_len>
LRUHash<str_len>::~LRUHash()
{
}

template <int str_len>
unsigned char* LRUHash<str_len>::Add(unsigned char* hashValue, const bool isDuplicated) {
	unsigned char* toBeDel;
	//Deal with the oldest hash value if the circular map is full
	toBeDel = circularQueue.Add(hashValue);
	if (toBeDel != nullptr) {
		if (map[toBeDel] == 1) {
			map.erase(toBeDel);
		}
		else {
			map[toBeDel] -= 1;
		}
	}
	if (isDuplicated) {
		//Use the newest char array as the key
		int occurence = map[hashValue];
		map.erase(hashValue);
		map[hashValue] = occurence + 1;
	}
	else {
		map[hashValue] = 1;
		//map.insert({ hashValue, 1 });
	}
	return toBeDel;
}

template <int str_len>
bool LRUHash<str_len>::Find(unsigned char* hashValue) {
	return map.find(hashValue) != map.end();
}

template <int str_len>
bool LRUHash<str_len>::FindAndAdd(unsigned char* hashValue, unsigned char* toBeDel) {
	charPtMap::iterator it = map.find(hashValue);
	bool isFound = it != map.end();
	toBeDel = circularQueue.Add(hashValue);
	if (toBeDel != nullptr) {
		charPtMap::iterator toBeDelIt = map.find(toBeDel);
		if (toBeDelIt->second == 1) {
			map.erase(toBeDelIt, toBeDelIt);
		}
		else {
			toBeDelIt->second -= 1;
		}
	}
	if (isFound) {
		//Use the newest char array as the key
		int occurence = it->second;
		map.erase(it, it);
		map[hashValue] = occurence + 1;
	}
	else {
		//map[hashValue] = 1;
		map.insert({ hashValue, 1 });
	}
	return isFound;
}