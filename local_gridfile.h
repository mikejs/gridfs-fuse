#ifndef _LOCAL_GRIDFILE_H
#define _LOCAL_GRIDFILE_H

#include <vector>
#include <cstring>

class LocalGridFile {
public:
    LocalGridFile(int chunkSize) :
      _chunkSize(chunkSize), _length(0) {
          _chunks.push_back(new char[256 * 1024]);
      }

    ~LocalGridFile() {
        for(std::vector<char*>::iterator i = _chunks.begin();
            i != _chunks.end(); i++) {
            delete *i;
        }
    }

    int getChunkSize() { return _chunkSize; }
    int getNumChunks() { return _chunks.size(); }
    int getLength() { return _length; }
    char* getChunk(int n) { return _chunks[n]; }

    int write(const char* buf, size_t nbyte, off_t offset);

private:
    int _chunkSize, _length;
    std::vector<char*> _chunks;
};

#endif
