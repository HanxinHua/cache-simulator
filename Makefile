all: cacheSimulator  

cacheSimulator: cacheSimulator.o
	g++ -O3 -o cacheSimulator cacheSimulator.o

cacheSimulator.o: cacheSimulator.cpp
	g++ -O3 -c -Wall cacheSimulator.cpp
	
clean:
	rm -rf *.o
