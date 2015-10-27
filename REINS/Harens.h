#pragma once
#include <cuda_runtime_api.h> 
#include <cuda.h>
#include "IO.h"
#include "LRUQueuePool.h"
#include "RabinHash.h"
#include "PcapReader.h"
#include "RedundancyEliminator_CUDA.h"
#include <cassert>

class Harens {
private:
	//constants
	int mapperNum = 8;
	int reducerNum = 256;	//Better be power of 2, it would make the module operation faster
	//determin if one thread is end
	bool read_file_end = false;
	/*bool transfer_end = false;*/
	bool chunking_kernel_end = false;
	bool chunking_proc_end = false;
	bool chunk_hashing_end = false;
	mutex read_file_end_mutex, chunking_kernel_end_mutex, chunking_proc_end_mutex, chunk_hashing_end_mutex;
	//file
	ifstream ifs;
	unsigned int file_length;
	FixedSizedCharArray charArrayBuffer;
	char overlap[WINDOW_SIZE - 1];
	//pagable buffer
	array<char*, PAGABLE_BUFFER_NUM> pagable_buffer;
	array<unsigned int, PAGABLE_BUFFER_NUM> pagable_buffer_len;
	array<mutex, PAGABLE_BUFFER_NUM> pagable_buffer_mutex;
	array<condition_variable, PAGABLE_BUFFER_NUM> pagable_buffer_cond;
	array<bool, PAGABLE_BUFFER_NUM> pagable_buffer_obsolete;
	//fixed buffer
	array<char*, FIXED_BUFFER_NUM> fixed_buffer;
	array<unsigned int, FIXED_BUFFER_NUM> fixed_buffer_len;
	/*array<mutex, FIXED_BUFFER_NUM> fixed_buffer_mutex;
	array<condition_variable, FIXED_BUFFER_NUM> fixed_buffer_cond;
	array<bool, FIXED_BUFFER_NUM> fixed_buffer_obsolete;*/
	//RedundancyEliminator_CUDA
	RedundancyEliminator_CUDA re;
	//chunking kernel asynchronize
	array<char*, FIXED_BUFFER_NUM> input_kernel;
	array<unsigned long long*, FIXED_BUFFER_NUM> result_kernel;
	array<unsigned long long*, FIXED_BUFFER_NUM> result_host;
	array<unsigned int, FIXED_BUFFER_NUM> result_host_len;
	array<mutex, FIXED_BUFFER_NUM> result_host_mutex;
	array<condition_variable, FIXED_BUFFER_NUM> result_host_cond;
	array<bool, FIXED_BUFFER_NUM> result_host_obsolete;
	array<bool, FIXED_BUFFER_NUM> result_host_executing;
	//chunking result processing
	array<cudaStream_t, RESULT_BUFFER_NUM> stream;
	array<unsigned int*, RESULT_BUFFER_NUM> chunking_result;
	array<unsigned int, RESULT_BUFFER_NUM> chunking_result_len;
	array<mutex, RESULT_BUFFER_NUM> chunking_result_mutex;
	array<condition_variable, RESULT_BUFFER_NUM> chunking_result_cond;
	array<bool, RESULT_BUFFER_NUM> chunking_result_obsolete;
	//chunk hashing
	thread *segment_threads;
	LRUQueuePool chunk_hash_queue_pool;
	//chunk matching 
	thread *chunk_match_threads;
	LRUHash *circ_hash_pool;
	unsigned int *duplication_size;
	unsigned int total_duplication_size = 0;
	//Time
	clock_t start, end, start_r, end_r, start_ck, end_ck, start_cp, end_cp, start_ch, end_ch, start_cm, end_cm;
	double time = 0, time_r = 0, time_ck = 0, time_cp = 0, time_ch, time_cm;
	int count = 0;

public:
	Harens(int mapperNum, int reducerNum);
	~Harens();

	void ReadFile();
	void Transfer();
	void ChunkingKernel();
	void ChunkingResultProc();
	void ChunkHashing();
	void ChunkMatch(int hashPoolIdx);

	void ChunkSegmentHashing(int pagableBufferIdx, int chunkingResultIdx, int segmentNum);

	int Execute();
};