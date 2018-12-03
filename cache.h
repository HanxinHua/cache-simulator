#include <iostream>
#include <unordered_map>
#include <vector>
#include <list>
#include <time.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

using std::list; 
using std::vector;
using std::cout;
using std::endl;
using std::unordered_map;
using std::stoi;
using std::ifstream;
using std::string;
using std::stringstream;

class configuration {
public:
	int dt;
	int l1, l2;
	vector<vector<int> > L1,L2;
	
	configuration(char * argv) {
		string line;
		ifstream myfile (argv);
		if (myfile.is_open()){
			getline (myfile,line);
			dt = stoi(line);
			getline (myfile,line);
			l1 = stoi(line);
			getline (myfile,line);
			l2 = stoi(line);
			L1.assign(l1, vector<int>(6,0));
			L2.assign(l2, vector<int>(6,0));
			int i = 0;
			int num;
			while ( i<l1 ) {
				
				getline (myfile,line);
				stringstream ss(line);
				for (int j =0; j<6; j++) {
					ss>>num;
					L1[i][j] = num;
				}
				i++;
			}
			i=0;
			while ( i<l2 ) {
				
				getline (myfile,line);
				stringstream ss(line);
				for (int j =0; j<6; j++) {
					ss>>num;
					L2[i][j] = num;
				}
				i++;
			}
			myfile.close();
		}
	}
};

class cache {
public:
	//A: ways
    //B: block_size
	//C: capacity
	int A, B, C; 
	int num_set;
	int way; // 0 represents LRU and 1 represents RND
	int aowm; //1: allocate on write miss and 0: not
	int level, l1, l2;
	vector<int> hitcount, misscount;
	vector<vector<int> > address;	
    vector<unordered_map<int, int> > set; //tag, A
    vector<list<int> > lrutag;
	vector<vector<bool> > v; //valid
	vector<vector<bool> > dirtybit;	
	cache * next, * next2;
	
    cache(int a, int b, int c, int w, int ao, int l, int n1, int n2, cache * n, cache * ntwo):A(a), B(b), C(c), way(w), aowm(ao), level(l), l1(n1), l2(n2),hitcount(vector<int>(3,0)), misscount(vector<int>(3,0)),address(vector<vector<int> >(C/B/A, vector<int>(A,0))), v(vector<vector<bool> >(C/B/A, vector<bool>(A,0))), dirtybit(vector<vector<bool> >(C/B/A, vector<bool>(A,0))) {
		num_set = C/B/A;
		set.assign(num_set, unordered_map<int, int>());
		lrutag.assign(num_set, list<int>());
		next = n;
		next2 = ntwo;
	}
	 
	int getIndex (int addr) {
		return (addr / B) % num_set;
	}
	 
	int getTag (int addr) {
		return addr / (num_set * B);
	}
    
	//way: 0 represents LRU and 1 represents RND
	int swapOne (int index, int tag, int way ) {
		 if (way) {
			srand(time(NULL));
			int randnum = rand()% A;
			auto it = set[index].begin();
			for (int i = 0; i<randnum; i++) it++;
			int waydex = it -> second;
			int addr = address[index][waydex];
			if (dirtybit[index][waydex]) {
				dirtybit[index][waydex] = 0;
				next->search (addr, 1); //writeback
			}
			set[index].erase(it);
			set[index][tag] = waydex;	
			return waydex;
		 }
		 else {
			int waydex = set[index][lrutag[index].front()];
			int addr = address[index][waydex];
			if (dirtybit[index][waydex]) {
				dirtybit[index][waydex] = 0;
				next->search (addr, 1); //writeback
			}
			set[index].erase(lrutag[index].front());
			set[index][tag] = waydex;
			lrutag[index].pop_front();
			lrutag[index].push_back(tag);
			return waydex;
		 }
		
	}
	
	void search (int addr,  int ctype) {
		if (level == 3) {//main memory
			hitcount[ctype]++;
			return;
		}
		int index = getIndex(addr);
		int tag = getTag(addr);
		int waydex;
		if (set[index].find(tag)!=set[index].end()) {//match the tag
			waydex = set[index][tag];
			if (v[index][waydex]) {//valid is true
				hitcount[ctype]++;
				if (ctype%2) dirtybit[index][waydex] = 1;
				address[index][waydex]=addr;
				lrutag[index].remove(tag);
				lrutag[index].push_back(tag);
			}
			else {
				misscount[ctype]++;
				if (level == 1 && l1 == 1 && l2 == 2 && ctype == 2) next2->search(addr, ctype);
				else next->search(addr, ctype);
				if (aowm || !(ctype%2)) {
					v[index][waydex]=1;
					address[index][waydex]=addr;
					lrutag[index].push_back(tag);
				}
			}
		}
		else {
			misscount[ctype]++;
			if (level == 1 && l1 == 1 && l2 == 2 && ctype == 2) next2->search(addr, ctype);
			else next->search(addr, ctype);
			if (aowm || !(ctype%2)) {
				if ((int)set[index].size()>=A) {//exceed the size of associativaty
					waydex = swapOne(index, tag, way);		
					address[index][waydex]=addr;
				}
				else {
					v[index][set[index].size()] = 1;
					address[index][set[index].size()]=addr;
					set[index][tag] = (set[index].size());
					lrutag[index].push_back(tag);
				}
			}
		}
	}
	
	
};

class cacheStructure {
public:  
  vector<cache*> top;
  struct result{
	  int fetch;
	  int miss;
	  double fot;
	  double missRate;
  };
  
  int l1, l2;
  int dramtime;
  vector<vector<int> > hittime;
  
  cacheStructure(vector<vector<int> >& L1, vector<vector<int> >& L2,int n1, int n2, int dtime ):l1(n1), l2(n2),dramtime(dtime), hittime(vector<vector<int> >(2, vector<int>(2,0))) {
	top.push_back(new cache(1,1,1,0,0, 3, l1, l2, NULL, NULL)); //main memory
	if (l2) {
		vector<cache*> ltwo;
		ltwo.resize(l2);
		ltwo[0] = new cache(L2[0][0], L2[0][1], L2[0][2], L2[0][3], L2[0][4], 2, l1, l2,top[0], NULL);
		hittime[1][0] = L2[0][5];
		if (l2 == 2) {
			ltwo[1] = new cache(L2[1][0], L2[1][1], L2[1][2], L2[1][3], L2[1][4],2, l1, l2, top[0], NULL);
			hittime[1][1] = L2[1][5];
		}
		top = ltwo;
	}
	if (l1) {
		vector<cache*> lone;
		lone.resize(l1);
		if (l2 == 2 && l1 == 1) lone[0] = new cache(L1[0][0], L1[0][1], L1[0][2], L1[0][3], L1[0][4],1, l1, l2, top[0], top[1]);
		else lone[0] = new cache(L1[0][0], L1[0][1], L1[0][2], L1[0][3], L1[0][4],1, l1, l2, top[0], NULL);
		hittime[0][0] = L1[0][5];
		if (l1 == 2) {
			hittime[0][1] = L1[1][5];
			if (l2 == 2) lone[1] = new cache(L1[1][0], L1[1][1], L1[1][2], L1[1][3], L1[1][4],1, l1, l2, top[1], NULL);
			else lone[1] = new cache(L1[1][0], L1[1][1], L1[1][2], L1[1][3], L1[1][4],1, l1, l2, top[0], NULL);
		}
		top = lone;
	}
  }
  
  void getAccess (int ctype, int addr) {
	  int entry = ctype/2*(l1-1);
	  top[entry]->search(addr, ctype);
  }
  void outputResults () {
	  vector<double> missRate, miss1, miss2;
	  double avetime11, avetime12, avetime21, avetime22;
	  cout<<endl;
	  if (l1 == 0) {
		  if (l2 == 0) {
			  cout<<"main memory"<<endl;
			  getResults(top[0]);
			  return;
		  }
		  else if (l2 == 1) {
			  cout<<"l2-cache"<<endl;
			  missRate = getResults(top[0]);
			  cout<<"Average time for l2-cache is "<<hittime[1][0]+missRate[0]*dramtime<<" cycles"<<endl;
			  return;
		  }
		  else {
			  cout<<"l2-icache"<<endl;
			  missRate = getResults(top[1]);
			  avetime22 = hittime[1][1]+missRate[0]*dramtime;
			  cout<<"l2-dcache"<<endl;
			  missRate = getResults(top[0]);
			  avetime21 = hittime[1][0]+missRate[0]*dramtime;
			  cout<<endl;
			  cout<<"Average time for l2-icache is "<<avetime22<<" cycles"<<endl;
			  cout<<"Average time for l2-dcache is "<<avetime21<<" cycles"<<endl;
			  
			  return;
		  }
	  }
	  else if (l1 == 1) {
		  cout<<"l1-cache"<<endl;
		  miss1 = getResults(top[0]);
		  if (l2 == 0) {
			  cout<<"Average time for l1-cache is "<<hittime[0][0]+miss1[0]*dramtime<<" cycles"<<endl;
			  return;
		  }
		  else if (l2 == 1) {
			  cout<<endl;
			  cout<<"l2-cache"<<endl;
			  missRate = getResults(top[0]->next);
			  avetime21 = hittime[1][0]+missRate[0]*dramtime;
			  avetime11 = hittime[0][0]+miss1[0]*avetime21;
			  cout<<endl<<"Average time for l1-cache is "<<avetime11<<" cycles"<<endl;
			  cout<<"Average time for l2-cache is "<<avetime21<<" cycles"<<endl;
			  return;
		  }
		  else {
			  cout<<endl;
			  cout<<"l2-icache"<<endl;
			  missRate = getResults(top[0]->next2);
			  cout<<endl;
			  cout<<"l2-dcache"<<endl;
			  missRate = getResults(top[0]->next);
			  avetime21 = hittime[1][0]+missRate[0]*dramtime;		  
			  avetime22 = hittime[1][1]+missRate[0]*dramtime;
			  avetime11 = hittime[0][0]+miss1[2]*avetime21+miss1[1]*avetime22;
			  cout<<endl<<"Average time for l1-cache is "<<avetime11<<" cycles"<<endl;
			  cout<<"Average time for l2-icache is "<<avetime22<<" cycles"<<endl;
			  cout<<"Average time for l2-dcache is "<<avetime21<<" cycles"<<endl;
			  
			  return;
		  }
	  
	  }
	  else {
		  cout<<endl<<"l1-icache"<<endl;
		  miss2 = getResults(top[1]);
		  cout<<endl<<"l1-dcache"<<endl;
		  miss1 = getResults(top[0]);		  
		  cout<<endl;
		  if (l2 == 0) {
			  cout<<"Average time for l1-icache is "<<hittime[0][1]+miss2[0]*dramtime<<" cycles"<<endl;
			  cout<<"Average time for l1-dcache is "<<hittime[0][0]+miss1[0]*dramtime<<" cycles"<<endl;	  
			  return;
		  }
		  else if (l2 == 1) {
			  cout<<"l2-cache"<<endl;
			  missRate = getResults(top[0]->next);
			  avetime21 = hittime[1][0]+missRate[0]*dramtime;
			  avetime11 = hittime[0][0]+miss1[0]*avetime21;
			  avetime12 = hittime[0][1]+miss2[0]*avetime21;
			  cout<<endl;
			  cout<<"Average time for l1-icache is "<<avetime12<<" cycles"<<endl;
			  cout<<"Average time for l1-dcache is "<<avetime11<<" cycles"<<endl;
			  cout<<"Average time for l2-cache is "<<avetime21<<" cycles"<<endl;
			  return;
		  }
		  else {
			  cout<<"l2-icache"<<endl;
			  missRate = getResults(top[1]->next);
			  cout<<endl;
			  cout<<"l2-dcache"<<endl;
			  missRate = getResults(top[0]->next);
			  avetime21 = hittime[1][0]+missRate[0]*dramtime;			  
			  avetime22 = hittime[1][1]+missRate[0]*dramtime;
			  avetime11 = hittime[0][0]+miss1[0]*avetime21;
			  avetime12 = hittime[0][1]+miss2[0]*avetime22;
			  cout<<endl;
			  cout<<"Average time for l1-icache is "<<avetime12<<" cycles"<<endl;
			  cout<<"Average time for l1-dcache is "<<avetime11<<" cycles"<<endl;
			  cout<<"Average time for l2-icache is "<<avetime22<<" cycles"<<endl;
			  cout<<"Average time for l2-dcache is "<<avetime21<<" cycles"<<endl;
			  
			  return;
		  }
	  }
		  
  }
  
  vector<double>  getResults (cache* c) {
	struct result total, instrn, data, read, write;
	
	instrn.fetch = c->hitcount[2] + c->misscount[2];
    instrn.miss = c->misscount[2];
    instrn.missRate = ( instrn.fetch != 0 ) ? (double)instrn.miss/instrn.fetch : 0;
	
	read.fetch = c->hitcount[0] + c->misscount[0];
    read.miss = c->misscount[0];
    read.missRate = (read.fetch!=0) ? (double)read.miss /read.fetch : 0;
	
	write.fetch = c->hitcount[1] + c->misscount[1];
    write.miss = c->misscount[1];
    write.missRate = (write.fetch != 0) ? (double)write.miss/write.fetch : 0;

    
    data.fetch = read.fetch + write.fetch;
    data.miss = read.miss + write.miss;    
    data.missRate = (data.fetch!=0) ? (double)data.miss /data.fetch : 0;
       
    total.fetch = data.fetch + instrn.fetch;
    total.miss = data.miss + instrn.miss;
    total.missRate =(total.fetch!=0) ? (double)total.miss /total.fetch : 0;

    instrn.fot = (double)instrn.fetch/total.fetch;
    read.fot = (double)read.fetch/total.fetch;
	write.fot = (double)write.fetch/total.fetch;
    data.fot = (double)data.fetch/total.fetch;    
    total.fot = (double)total.fetch/total.fetch;

	 
	std::cout.precision(4); 

    cout <<"Metrics          "<<"\t\tTotal "<<"\t\tInstrn "<<"\t\tData  "<<"\t\tRead  "<<"\t\tWrite "<<"\t\tMisc"<<endl;
	cout <<"-----------------"<<"\t\t------"<<"\t\t------"<<"\t\t------"<<"\t\t------"<<"\t\t------"<<"\t\t------"<<endl;
	cout <<std::showpoint<<std::fixed<<"Demand Fetches   "<<"\t\t"<<total.fetch<<"\t\t"<<instrn.fetch<<"\t\t"<<data.fetch<<"\t\t"<<read.fetch<<"\t\t"<<write.fetch<<"\t\t0"<<endl;
	cout <<std::showpoint<<std::fixed<<" Fraction of Total"<<"\t\t"<<total.fot<<"\t\t"<<instrn.fot<<"\t\t"<<data.fot<<"\t\t"<<read.fot<<"\t\t"<<write.fot<<"\t\t0.0000"<<endl;
    cout <<endl;
	cout <<std::showpoint<<std::fixed<<"Demand Misses    "<<"\t\t"<<total.miss<<"\t\t"<<instrn.miss<<"\t\t"<<data.miss<<"\t\t"<<read.miss<<"\t\t"<<write.miss<<"\t\t0"<<endl;
    cout <<std::showpoint<<std::fixed<<" Demond miss rate "<<"\t\t"<<total.missRate<<"\t\t"<<instrn.missRate<<"\t\t"<<data.missRate<<"\t\t"<<read.missRate<<"\t\t"<<write.missRate<<"\t\t0.0000"<<endl;
	vector<double> m = {total.missRate, instrn.missRate, data.missRate};
	return m;
  }
  
  ~cacheStructure(){
	  if (l1 == 0 && l2 == 0) delete top[0];
	  else if (l1 == 0) {
		  delete top[0]->next;
		  for (int i = 0; i< l1; i++) delete top[i];
	  }
	  else if (l2 == 0) {
		  delete top[0]->next;
		  for (int i = 0; i< l2; i++) delete top[i];
	  }
	  else {
		  delete top[0]->next->next;
		  if (l2 == 2) delete top[0]->next2;
		  delete top[0]->next;
		  if (l1 == 2) delete top[1];
		  delete top[0];
	  }
  }
};
