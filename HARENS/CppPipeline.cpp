#include "CppPipeline.h"
using namespace std;

CppPipeline::CppPipeline(): charArrayBuffer(MAX_BUFFER_LEN) {
	re.SetupRedundancyEliminator_CPP();

	buffer = new char*[PAGABLE_BUFFER_NUM];
	for (int i = 0; i < PAGABLE_BUFFER_NUM; ++i) {
		buffer[i] = new char[MAX_BUFFER_LEN];
		buffer_len[i] = 0;
		buffer_obsolete[i] = true;
	}

	for (int i = 0; i < RESULT_BUFFER_NUM; ++i) {
		chunking_result_obsolete[i] = true;
	}
}

CppPipeline::~CppPipeline() {
	//delete everything that mallocated before
	for (int i = 0; i < PAGABLE_BUFFER_NUM; ++i)
		delete buffer[i];
	delete[] buffer;
}

int CppPipeline::Execute()
{
	IO::Print("\n================== Pipeline Version of C++ Implementation ===================\n");

	//Create threads 
	thread tReadFile(std::mem_fn(&CppPipeline::ReadFile), this);
	tReadFile.join();
	clock_t start = clock();
	thread tChunking(std::mem_fn(&CppPipeline::Chunking), this);
	thread tFingerprinting(std::mem_fn(&CppPipeline::Fingerprinting), this);

	tChunking.join();
	tFingerprinting.join();

	IO::Print("Found %s of redundency, which is %f %% of file\n"
		, IO::InterpretSize(total_duplication_size)
		, (float)total_duplication_size / file_length * 100);

	clock_t end = clock();
	IO::Print("Reading time: %f ms\n", tot_read);
	IO::Print("Chunking time: %f ms\n", tot_chunk);
	IO::Print("Fingerprinting time: %f ms\n", tot_fin);
	IO::Print("Total time: %f ms\n", ((float)end - start) * 1000 / CLOCKS_PER_SEC);
	IO::Print("=============================================================================\n");

	return 0;
}

void CppPipeline::ReadFile() {
	int bufferIdx = 0;
	unsigned int curFilePos = 0;
	int curWindowNum;
	PcapReader fileReader;
	//Read the first part
	unique_lock<mutex> readFileInitLock(buffer_mutex[bufferIdx]);
	start_read = clock();
	if (IO::FILE_FORMAT == PlainText) {
		ifs = ifstream(IO::input_file_name, ios::in | ios::binary | ios::ate);
		if (!ifs.is_open()) {
			printf("Can not open file %s\n", IO::input_file_name);
			return;
		}

		file_length = ifs.tellg();
		ifs.seekg(0, ifs.beg);
		IO::Print("File size: %s\n", IO::InterpretSize(file_length));
		buffer_len[bufferIdx] = min(MAX_BUFFER_LEN, file_length - curFilePos);
		curWindowNum = buffer_len[bufferIdx] - WINDOW_SIZE + 1;
		ifs.read(buffer[bufferIdx], buffer_len[bufferIdx]);
		curFilePos += curWindowNum;
	}
	else if (IO::FILE_FORMAT == Pcap) {
		fileReader.SetupPcapHandle(IO::input_file_name);
		fileReader.ReadPcapFileChunk(charArrayBuffer, MAX_BUFFER_LEN);
		buffer_len[bufferIdx] = charArrayBuffer.GetLen();
		memcpy(buffer[bufferIdx], charArrayBuffer.GetArr(), buffer_len[bufferIdx]);
		file_length += buffer_len[bufferIdx];
	}
	else
		fprintf(stderr, "Unknown file format\n");
		
	memcpy(overlap, &buffer[bufferIdx][buffer_len[bufferIdx] - WINDOW_SIZE + 1], WINDOW_SIZE - 1);	//copy the last window into overlap
	buffer_obsolete[bufferIdx] = false;
	readFileInitLock.unlock();
	buffer_cond[bufferIdx].notify_one();
	bufferIdx = (bufferIdx + 1) % PAGABLE_BUFFER_NUM;
	tot_read += ((float)clock() - start_read) * 1000 / CLOCKS_PER_SEC;
	//Read the rest
	if (IO::FILE_FORMAT == PlainText) {
		while (curWindowNum == MAX_WINDOW_NUM) {
			unique_lock<mutex> readFileIterLock(buffer_mutex[bufferIdx]);
			while (buffer_obsolete[bufferIdx] == false) {
				buffer_cond[bufferIdx].wait(readFileIterLock);
			}
			start_read = clock();
			buffer_len[bufferIdx] = min(MAX_BUFFER_LEN, file_length - curFilePos + WINDOW_SIZE - 1);
			curWindowNum = buffer_len[bufferIdx] - WINDOW_SIZE + 1;
			memcpy(buffer[bufferIdx], overlap, WINDOW_SIZE - 1);	//copy the overlap into current part
			ifs.read(&buffer[bufferIdx][WINDOW_SIZE - 1], curWindowNum);
			memcpy(overlap, &buffer[bufferIdx][curWindowNum], WINDOW_SIZE - 1);	//copy the last window into overlap
			buffer_obsolete[bufferIdx] = false;
			readFileIterLock.unlock();
			buffer_cond[bufferIdx].notify_one();
			bufferIdx = (bufferIdx + 1) % PAGABLE_BUFFER_NUM;
			curFilePos += curWindowNum;
			tot_read += ((float)clock() - start_read) * 1000 / CLOCKS_PER_SEC;
		}
		ifs.close();
	}
	else if (IO::FILE_FORMAT == Pcap) {
		while (true) {
			unique_lock<mutex> readFileIterLock(buffer_mutex[bufferIdx]);
			while (buffer_obsolete[bufferIdx] == false) {
				buffer_cond[bufferIdx].wait(readFileIterLock);
			}
			start_read = clock();
			fileReader.ReadPcapFileChunk(charArrayBuffer, MAX_BUFFER_LEN - WINDOW_SIZE + 1);
				
			if (charArrayBuffer.GetLen() == 0) {
				readFileIterLock.unlock();
				buffer_cond[bufferIdx].notify_all();
				break;	//Read nothing
			}
			memcpy(buffer[bufferIdx], overlap, WINDOW_SIZE - 1);	//copy the overlap into current part
			memcpy(&buffer[bufferIdx][WINDOW_SIZE - 1], charArrayBuffer.GetArr(), charArrayBuffer.GetLen());
			buffer_len[bufferIdx] = charArrayBuffer.GetLen() + WINDOW_SIZE - 1;
			file_length += charArrayBuffer.GetLen();
			buffer_obsolete[bufferIdx] = false;
			memcpy(overlap, &buffer[bufferIdx][charArrayBuffer.GetLen()], WINDOW_SIZE - 1);	//copy the last window into overlap
			readFileIterLock.unlock();
			buffer_cond[bufferIdx].notify_one();
			bufferIdx = (bufferIdx + 1) % PAGABLE_BUFFER_NUM;
			tot_read += ((float)clock() - start_read) * 1000 / CLOCKS_PER_SEC;
		}
		IO::Print("File size: %s\n", IO::InterpretSize(file_length));
	}
	else
		fprintf(stderr, "Unknown file format\n");
	unique_lock<mutex> readFileEndLock(read_file_end_mutex);
	read_file_end = true;
	//In case the other threads stuck in waiting for condition variable
	buffer_cond[bufferIdx].notify_all();
}

void CppPipeline::Chunking() {
	int bufferIdx = 0;
	int chunkingResultIdx = 0;
	while (true) {
		unique_lock<mutex> bufferLock(buffer_mutex[bufferIdx]);
		if (buffer_obsolete[bufferIdx]) {
			unique_lock<mutex> readFileEndLock(read_file_end_mutex);
			if (read_file_end){
				unique_lock<mutex> chunkingEndLock(chunking_end_mutex);
				chunking_end = true;
				return;
			}
			readFileEndLock.unlock();
			buffer_cond[bufferIdx].wait(bufferLock);
		}

		start_chunk = clock();
		deque<unsigned int> currentChunkingResult = re.chunking(buffer[bufferIdx], buffer_len[bufferIdx]);
		tot_chunk += ((float)clock() - start_chunk) * 1000 / CLOCKS_PER_SEC;
		bufferLock.unlock();
		buffer_cond[bufferIdx].notify_one();
		bufferIdx = (bufferIdx + 1) % PAGABLE_BUFFER_NUM;

		unique_lock<mutex> chunkingResultLock(chunking_result_mutex[chunkingResultIdx]);
		while (chunking_result_obsolete[chunkingResultIdx] == false) {
			chunking_result_cond[chunkingResultIdx].wait(chunkingResultLock);
		}
		chunking_result[chunkingResultIdx] = currentChunkingResult;
		chunking_result_obsolete[chunkingResultIdx] = false;
		chunkingResultLock.unlock();
		chunking_result_cond[chunkingResultIdx].notify_one();
		chunkingResultIdx = (chunkingResultIdx + 1) % RESULT_BUFFER_NUM;

	}
}

void CppPipeline::Fingerprinting() {
	int bufferIdx = 0;
	int chunkingResultIdx = 0;
	//When the whole process starts, all chunking results are obsolete, that's the reason fingerprinting part need to check buffer state
	while (true) {
		//Get buffer ready
		unique_lock<mutex> bufferLock(buffer_mutex[bufferIdx]);
		while (buffer_obsolete[bufferIdx]) {
			unique_lock<mutex> chunkingEndLock(chunking_end_mutex);
			if (chunking_end)
				return;
			chunkingEndLock.unlock();
			buffer_cond[bufferIdx].wait(bufferLock);
		}
		//Get chunking result ready
		unique_lock<mutex> chunkingResultLock(chunking_result_mutex[chunkingResultIdx]);
		while (chunking_result_obsolete[chunkingResultIdx]) {
			unique_lock<mutex> chunkingEndLock(chunking_end_mutex);
			if (chunking_end)
				return;
			chunkingEndLock.unlock();
			chunking_result_cond[chunkingResultIdx].wait(chunkingResultLock);
		}

		start_fin = clock();

		total_duplication_size += re.fingerPrinting(chunking_result[chunkingResultIdx], buffer[bufferIdx]);
		buffer_obsolete[bufferIdx] = true;
		chunking_result_obsolete[chunkingResultIdx] = true;
		bufferLock.unlock();
		buffer_cond[bufferIdx].notify_one();
		chunkingResultLock.unlock();
		chunking_result_cond[chunkingResultIdx].notify_one();

		bufferIdx = (bufferIdx + 1) % PAGABLE_BUFFER_NUM;
		chunkingResultIdx = (chunkingResultIdx + 1) % RESULT_BUFFER_NUM;
		tot_fin += ((float)clock() - start_fin) * 1000 / CLOCKS_PER_SEC;
	}
}