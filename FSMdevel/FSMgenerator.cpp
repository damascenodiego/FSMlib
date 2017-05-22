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
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>

#include "../FSMlib/FSMlib.h"

using namespace FSMsequence;
using namespace FSMtesting;

#define DATA_PATH			string("../data/")
#define MINIMIZATION_DIR	string("tests/minimization/")
#define SEQUENCES_DIR		string("tests/sequences/")
#define EXAMPLES_DIR		string("examples/")
#define EXPERIMENTS_DIR		string("experiments/")
#define OUTPUT_GV			string(DATA_PATH + "tmp/output.gv").c_str()

using namespace std::tr2::sys;

static bool checkSolution(vector<unique_ptr<DFSM>>& indistinguishable, const unique_ptr<DFSM>& partial) {
	auto accurate = FSMmodel::duplicateFSM(partial);
	if (!accurate->isReduced()) accurate->minimize();
	for (auto &f : indistinguishable) {
		if (FSMmodel::areIsomorphic(f, accurate)) {
			return false;
		}
	}
	indistinguishable.emplace_back(move(accurate));
	return true;
}

void generateMachines(int argc, char** argv) {
	auto dir = string(argv[1]);
	unsigned int machineTypeMask = unsigned int(-1);// all
	size_t numMachines(10);
	state_t Nmin(10), Nstep(1), Nmax(10);
	input_t INmin(2), INstep(1), INmax(2), INmultiN(0);
	output_t OUTmin(2), OUTstep(1), OUTmax(2), OUTmultiN(0);
	bool reqMinimized = true;
	bool reqStronglyConnected = true;
	bool reqADS = false;
	bool checkEquivalence = true;
	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-m") == 0) {//machine type
			machineTypeMask = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-n") == 0) {//number of machines
			numMachines = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-s") == 0) {//number of states
			Nmin = Nmax = state_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-sr") == 0) {//number of states as range
			Nmin = state_t(atoi(argv[++i]));
			Nstep = state_t(atoi(argv[++i]));
			Nmax = state_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-i") == 0) {//number of inputs
			INmin = INmax = input_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-ir") == 0) {//number of inputs as range
			INmin = input_t(atoi(argv[++i]));
			INstep = input_t(atoi(argv[++i]));
			INmax = input_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-im") == 0) {//number of inputs as multiple of states
			INmultiN = input_t(atoi(argv[++i]));
			INmin = INmax = 0;
		}
		else if (strcmp(argv[i], "-o") == 0) {//number of outputs
			OUTmin = OUTmax = output_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-or") == 0) {//number of outputs as range
			OUTmin = output_t(atoi(argv[++i]));
			OUTstep = output_t(atoi(argv[++i]));
			OUTmax = output_t(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-om") == 0) {//number of outputs as multiple of states
			OUTmultiN = output_t(atoi(argv[++i]));
			OUTmin = OUTmax = 0;
		}
		else if (strcmp(argv[i], "-re") == 0) {//reduced machines?
			reqMinimized = bool(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-sc") == 0) {//strongly connected machines?
			reqStronglyConnected = bool(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-ads") == 0) {//machines with ADS?
			reqADS = bool(atoi(argv[++i]));
		}
		else if (strcmp(argv[i], "-d") == 0) {//check equivalence/difference of machines?
			checkEquivalence = bool(atoi(argv[++i]));
		}
	}
	if (dir.back() != '/') dir.push_back('/');

	srand(time(NULL));
	printf("DifferentMachines\tFSMtype\tStates\tInputs\tOutputs\t"
		"Generated\tDuplicates\tNoADS\tNotStronglyConnected\tUnreduced\n");
	size_t cUnreduced, cNotSC, cNoADS, cEq, cSat;
	vector<unique_ptr<DFSM>> generated;
	for (machine_type_t machineType = TYPE_DFSM; machineType <= TYPE_DFA; machineType++) {
		if (machineTypeMask & (1 << machineType)) {
			unique_ptr<DFSM> fsm;
			switch (machineType) {
			case TYPE_DFSM:
				fsm = move(make_unique<DFSM>());
				break;
			case TYPE_MEALY:
				fsm = move(make_unique<Mealy>());
				break;
			case TYPE_MOORE:
				fsm = move(make_unique<Moore>());
				break;
			case TYPE_DFA:
				fsm = move(make_unique<DFA>());
				break;
			}
			for (state_t states = Nmin; states <= Nmax; states += Nstep) {
				for (input_t inputs = INmin + INmultiN * states; inputs <= INmax + INmultiN * states; inputs += INstep) {
					for (output_t outputs = OUTmin + OUTmultiN * states; outputs <= OUTmax + OUTmultiN * states; outputs += OUTstep) {
						cSat = cEq = cNoADS = cNotSC = cUnreduced = 0;
						generated.clear();
						while (cSat < numMachines) {
							fsm->generate(states, inputs, outputs);
							if (machineType == TYPE_DFSM) {
								bool properDFSM = false;
								for (state_t state = 0; !properDFSM && (state < states); state++) {
									for (input_t i = 0; i < inputs; i++) {
										if (fsm->getOutput(state, i) != fsm->getOutput(fsm->getNextState(state, i), STOUT_INPUT)) {
											properDFSM = true;
											break;
										}
									}
								}
								if (!properDFSM) continue;
							}
							if (reqMinimized) fsm->minimize();
							if (states == fsm->getNumberOfStates()) {
								if (!reqStronglyConnected || FSMmodel::isStronglyConnected(fsm)) {
									if (!reqADS || FSMsequence::getAdaptiveDistinguishingSequence(fsm)) {
										if (!checkEquivalence || checkSolution(generated, fsm)) {
											fsm->save(dir);
											cSat++;
										}
										else {
											cEq++;
										}
									}
									else {
										//fsm->save(dir + "NoADS/");
										cNoADS++;
									}
								}
								else {
									//fsm->save(dir + "NotStronglyConnected/");
									cNotSC++;
								}
							}
							else {
								cUnreduced++;
							}			
						}
						printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", checkEquivalence, machineType,
							fsm->getNumberOfStates(), fsm->getNumberOfInputs(), fsm->getNumberOfOutputs(),
							numMachines, 
							checkEquivalence ? cEq : -1, 
							reqADS ? cNoADS : -1,
							reqStronglyConnected ? cNotSC : -1,
							reqMinimized ? cUnreduced : -1);
					}
				}
			}
		}
	}
	printf("complete.\n");
}

