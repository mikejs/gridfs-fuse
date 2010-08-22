CC=g++
CCOPTS=-g -D_FILE_OFFSET_BITS=64 -I. -I/usr/local/include
LDOPTS=-L/usr/local/lib -L. -lmongoclient -lfuse_ino64 -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt

mount_gridfs : operations.o local_gridfile.o main.o options.o
	$(CC) -o mount_gridfs main.o operations.o options.o local_gridfile.o $(LDOPTS)

operations.o : operations.cpp operations.h
	$(CC) $(CCOPTS) -c operations.cpp

local_gridfile.o : local_gridfile.cpp local_gridfile.h
	$(CC) $(CCOPTS) -c local_gridfile.cpp

main.o : main.cpp
	$(CC) $(CCOPTS) -c main.cpp

options.o : options.cpp options.h
	$(CC) $(CCOPTS) -c options.cpp

clean:
	rm -f mount_gridfs main.o operations.o options.o local_gridfile.o
