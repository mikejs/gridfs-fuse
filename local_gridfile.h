#ifndef _LOCAL_GRIDFILE_H
#define _LOCAL_GRIDFILE_H

#include <vector>
#include <cstring>
#include <iostream>

const unsigned int DEFAULT_CHUNK_SIZE = 256 * 1024;

class LocalGridFile {
public:
  LocalGridFile(int chunkSize = DEFAULT_CHUNK_SIZE) :
  _chunkSize(chunkSize), _length(0), _dirty(true) {
      _chunks.push_back(new char[_chunkSize]);
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
  bool dirty() { return _dirty; }
  void flushed() { _dirty = false; }

  int write(const char* buf, size_t nbyte, off_t offset);
  int read(char* buf, size_t size, off_t offset);

private:
  int _chunkSize, _length;
  bool _dirty;
  std::vector<char*> _chunks;
};

#endif
