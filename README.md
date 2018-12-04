# cache-simulator

./cacheSimulator configuration.txt DineroFull.din.txt

The first input file is used to get the configuration of the whole memory. It defines the number of caches and the type of each: None/Unified/Split. It also includes A, B, C, allocate on write miss or not, LRU/RND, hittime.

The second input file is the execution file includes the command type and the address.

This code also deals with different kind of miss types: compulsory miss, capacity miss, and conflict miss.
Example can be seen below.

![alt text](https://raw.githubusercontent.com/HanxinHua/cache-simulator/master/1.png)
![alt text](https://raw.githubusercontent.com/HanxinHua/cache-simulator/master/2.png)
