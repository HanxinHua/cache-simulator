#include <iostream>
#include <fstream>
#include <string>
#include "cache.h"

using std::cout;
using std::endl;
using std::stoi;
using std::ifstream;
using std::string;


void simulate(cacheStructure * cs, char * argv) {
	string line;
	ifstream myfile (argv);
	if (myfile.is_open()){
		while (getline (myfile,line)) {
			string ctype , addr;
			ctype = line.substr(0,1);
			addr = line.substr(2,line.size()-2);
			cs->getAccess(stoi(ctype), stoi(addr, nullptr ,16)); 
		}
	}		
	myfile.close();
	cs->outputResults();
}

int main (int argc, char ** argv) {
	if (argc !=3) {
		cout<<"./cacheSimulator configuration.txt Dinero.txt"<<endl;
		exit(EXIT_FAILURE);
	}
	configuration * c = new configuration(argv[1]);
	cacheStructure * cs = new cacheStructure (c->L1, c->L2, c->l1, c->l2, c->dt);
	simulate(cs, argv[2]);
	return 0;	
}
