/* Copyright (c) Michal Soucha, 2016
*
* This file is part of FSMlib
*
* FSMlib is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* FSMlib is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* FSMlib. If not, see <http://www.gnu.org/licenses/>.
*/

#define DBG_MEMORY_LEAK 0
#if DBG_MEMORY_LEAK
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>  
#endif
#ifndef PARALLEL_COMPUTING
//#define PARALLEL_COMPUTING // un/comment this if CUDA is enabled/disabled
#endif // !PARALLEL_COMPUTING

#include "commons.h"

unique_ptr<DFSM> fsm;

#define COMPUTATION_TIME(com) \
	auto start = chrono::system_clock::now(); \
	com; \
	auto end = chrono::system_clock::now(); \
	chrono::duration<double> elapsed_seconds = end - start;\
	printf("Time elapsed (in seconds):\t%f\n",elapsed_seconds.count());

static void printTS(sequence_set_t & TS, string filename) {
	FSMlib::PrefixSet ps;
	seq_len_t len(0);
	int test_id = 0;
	for (const auto& cSeq : TS) {
		len += cSeq.size();
		ps.insert(cSeq);
		printf("Test sequence_%d:\t%s\n", test_id, FSMmodel::getInSequenceAsString(cSeq).c_str());
		test_id+=1;
	}
	std::cout << "Number of resets:\t" << TS.size() << std::endl;
	std::cout << "Total length:\t" << len << std::endl;
	//auto syms = ps.getNumberOfSymbols();
	//printf("%d,%d,%d,%f,%d,%f\n", TS.size(), len, TS.size()+len, double(len)/TS.size(), syms, double(syms)/len);
}

int main(int argc, char** argv) {
	int ES = 0;
	string fileName = string(argv[1]);
	string method = "w";

	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-es") == 0) {//number of extra states
			ES = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-method") == 0) {//testing method
			method = string(argv[++i]);
		}
		else if (strcmp(argv[i], "-help") == 0) {//help menu
			std::cout << "FSMlib <FSM filename> -es [Number of extra states] -method [w,wp,hsi,h,spy,spyh]" << fileName << std::endl;
			return 0;
		}
	}
	
	std::cout << "FSM name:\t" << fileName << std::endl;
	std::cout << "Testing method:\t" << method << std::endl;
	std::cout << "Number of extra states:\t" << ES << std::endl;
	
	fsm = FSMmodel::loadFSM(fileName);

	if (method.compare("w")) {// generate tests using the w method
		COMPUTATION_TIME(auto TS = W_method(fsm, ES););  printTS(TS, fileName);
	}
	else if (method.compare("wp")) {// generate tests using the wp method
		COMPUTATION_TIME(auto TS = Wp_method(fsm, ES););  printTS(TS, fileName);
	}
	else if (method.compare("hsi")) {// generate tests using the hsi method
		COMPUTATION_TIME(auto TS = HSI_method(fsm, ES););  printTS(TS, fileName);
	}
	else if (method.compare("h")) {// generate tests using the h method
		COMPUTATION_TIME(auto TS = H_method(fsm, ES););  printTS(TS, fileName);
	}
	else if (method.compare("spy")) {// generate tests using the spy method
		COMPUTATION_TIME(auto TS = SPY_method(fsm, ES););  printTS(TS, fileName);
	}
	else if (method.compare("spyh")) {// generate tests using the spyh method
		COMPUTATION_TIME(auto TS = SPYH_method(fsm, ES););  printTS(TS, fileName);
	}
	// else if (method.compare("s")) {// generate tests using the s method
	// 	COMPUTATION_TIME(auto TS = S_method(fsm, ES););  printTS(TS, fileName);
	// }
	// else {
	// 	auto st = getSplittingTree(fsm, true);
	// 	auto hsiST = getHarmonizedStateIdentifiersFromSplittingTree(fsm, st);
	// 	if (method.compare("hsi_st")) {// generate tests using the hsi method using splitting tree 
	// 		COMPUTATION_TIME(auto TS = HSI_method(fsm, ES, hsiST););  printTS(TS, fileName);
	// 	}
	// 	else if (method.compare("spy_st")) {// generate tests using the spy method using splitting tree 
	// 		COMPUTATION_TIME(auto TS = SPY_method(fsm, ES, hsiST););  printTS(TS, fileName);
	// 	}

	// }
	return 0;
}