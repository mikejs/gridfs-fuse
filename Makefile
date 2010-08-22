CC=g++
CCOPTS=-D_FILE_OFFSET_BITS=64 -I. -I/usr/local/include
LDOPTS=-L/usr/local/lib -L. -lmongoclient -lfuse -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt

mount_gridfs : operations.o local_gridfile.o main.o options.o
	$(CC) $(LDOPTS) -o mount_gridfs main.o operations.o options.o local_gridfile.o

operations.o :
	$(CC) $(CCOPTS) -c operations.cpp

local_gridfile.o :
	$(CC) $(CCOPTS) -c local_gridfile.cpp

main.o :
	$(CC) $(CCOPTS) -c main.cpp

options.o :
	$(CC) $(CCOPTS) -c options.cpp
