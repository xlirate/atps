CC=g++
CFLAGS=-std=c++17

INCLUDECADMIUM=-I ../cadmium/include
INCLUDEDESTIMES=-I ../DESTimes/include -I ./vendor
INCLUDEJSON=-I ../cadmium/json/include
#INCLUDEBOOST=-I /home/thomas/boost/boost
VARIABLES=#-DNDEBUG

default: 1d_4p_4v_test

#CREATE BIN AND BUILD FOLDERS TO SAVE THE COMPILED FILES DURING RUNTIME
bin_folder := $(shell mkdir -p bin)
build_folder := $(shell mkdir -p build)
results_folder := $(shell mkdir -p simulation_results)



1d_4p_4v_test.o:
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) tests/1d_4p_4v_test.cpp -o build/1d_4p_4v_test.o
1d_4p_4v_test: 1d_4p_4v_test.o
	$(CC) $(VARIABLES) -g -o bin/1d_4p_4v_test.out build/1d_4p_4v_test.o



1d_4p_4v_infinit_test.o:
	$(CC) -g -c $(CFLAGS) $(INCLUDECADMIUM) $(INCLUDEDESTIMES) $(INCLUDEJSON) $(VARIABLES) tests/1d_4p_4v_infinit_test.cpp -o build/1d_4p_4v_infinit_test.o
1d_4p_4v_infinit_test: 1d_4p_4v_infinit_test.o
	$(CC) $(VARIABLES) -g -o bin/1d_4p_4v_infinit_test.out build/1d_4p_4v_infinit_test.o



clean:
	rm -f bin/* build/*


all: clean main
