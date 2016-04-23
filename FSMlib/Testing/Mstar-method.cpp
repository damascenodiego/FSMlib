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
#include "stdafx.h"

#include <sys/stat.h>
#include <direct.h>
#include "FSMtesting.h"
#include "../UnionFind.h"

using namespace FSMsequence;

namespace FSMtesting {

#define GUROBI_SOLVER string("/bin/gurobi_cl")
#define LP_FOLDER "lp"

	struct pq_entry_t {
		int from, to, len;

		pq_entry_t(int from, int to, int len) :
			from(from), len(len), to(to) {
		}
	};

	struct pq_entry_comp {

		bool operator()(const pq_entry_t& x, const pq_entry_t& y) const {
			return x.len > y.len;
		}
	};

	static bool equalSeqPart(sequence_in_t::iterator first1, sequence_in_t::iterator last1,
		sequence_in_t::iterator first2, sequence_in_t::iterator last2) {
		while ((first1 != last1) && (first2 != last2)) {
			if (!(*first1 == *first2))
				return false;
			++first1;
			++first2;
		}
		return true;
	}

	static void printCosts(vector<vector<seq_len_t> >& costs) {
		int Tsize = costs.size();
		printf("\t");
		for (int i = 0; i < Tsize; i++) {
			printf("%d\t", i);
		}
		printf("\n");
		for (int i = 0; i < Tsize; i++) {
			printf("%d\t", i);
			for (int j = 0; j < Tsize; j++) {
				printf("%d\t", costs[i][j]);
			}
			printf("\n");
		}
	}

	static bool writeLP(vector<vector<seq_len_t> >& costs, string fileName) {
		FILE * file = NULL;
		if (!(file = fopen(fileName.c_str(), "w"))) {
			ERROR_MESSAGE("Mstar-method: Unable to create a file for LP.");
			return false;
		}
		int numTests = costs.size();
		fprintf(file, "Minimize\n\t");
		for (int i = 0; i < numTests - 1; i++) {
			for (int j = 0; j < numTests; j++) {
				fprintf(file, "%d x_%d_%d + ", costs[i][j], i, j);
			}
			fprintf(file, "\n\t");
		}
		for (int j = 0; j < numTests - 1; j++) {
			fprintf(file, "%d x_%d_%d + ", costs[numTests - 1][j], numTests - 1, j);
		}
		fprintf(file, "%d x_%d_%d\n", costs[numTests - 1][numTests - 1], numTests - 1, numTests - 1);

		fprintf(file, "Subject To\n\t");
		//enter once
		for (int j = 0; j < numTests; j++) {
			for (int i = 0; i < numTests - 1; i++) {
				fprintf(file, "x_%d_%d + ", i, j);
			}
			fprintf(file, "x_%d_%d = 1\n\t", numTests - 1, j);
		}
		//leave once
		for (int i = 0; i < numTests; i++) {
			for (int j = 0; j < numTests - 1; j++) {
				fprintf(file, "x_%d_%d + ", i, j);
			}
			fprintf(file, "x_%d_%d = 1\n\t", i, numTests - 1);
		}
		// cycle indivisibility
		for (int i = 0; i < numTests; i++) {
			for (int j = 0; j < numTests - 1; j++) {
				fprintf(file, "o_%d - o_%d - %d x_%d_%d >= %d\n\t", j, i, 2 * numTests, i, j, (1 - 2 * numTests));
			}
		}
		//fprintf(file, "o_%d = 1\n", numTests - 1);

		fprintf(file, "Binaries\n\t");
		for (int i = 0; i < numTests; i++) {
			for (int j = 0; j < numTests; j++) {
				fprintf(file, "x_%d_%d ", i, j);
			}
		}
		fprintf(file, "\nEnd\n");
		fflush(file);
		fclose(file);
		return true;
	}

	static void getSolution(vector<state_t> & order, string fileName) {
		//sleep(1);
		struct stat buffer;
		while (stat(fileName.c_str(), &buffer) != 0) {
			Sleep(1);
		}
		ifstream fin(fileName);
		string s;
		int numTests = order.size(), pos;
		getline(fin, s);
		for (int i = 0; i < numTests; i++) {
			for (int j = 0; j < numTests; j++) {
				fin >> s >> pos;
				if (pos) {
					order[i] = j;
				}
			}
		}
		/*
		for (int i = 0; i < numTests*numTests; i++) getline(fin, s);
		for (int i = 0; i < numTests; i++) {
		fin >> s >> pos;
		if ((pos < 0) || (pos >= numTests)) {
		throw("Unable to read solution "+ s);
		}
		order[pos] = i;
		//printf("%s %d\n",s.c_str(),pos);
		}*/
	}

	static bool process_Mstar(DFSM* fsm, sequence_set_t& TS, int extraStates, bool resetEnabled) {
		TS.clear();
		char* gurobiPath;
		gurobiPath = getenv("GUROBI_HOME");
		if (gurobiPath == NULL) {
			ERROR_MESSAGE("Mstar-method needs Gurobi solver to run and GUROBI_HOME is not a system variable!");
			return false;
		}
		
		AdaptiveDS* ADS;
		if (!getAdaptiveDistinguishingSequence(fsm, ADS)) {
			return false;
		}
		state_t N = fsm->getNumberOfStates(), P = fsm->getNumberOfInputs();
		sequence_vec_t d(N);
		sequence_in_t CS;
		auto states = fsm->getStates();

		getADSet(fsm, ADS, d);
		delete ADS;

		if (fsm->isOutputState()) {
			for (state_t i = 0; i < d.size(); i++) {
				auto origDS = d[i];
				auto DSit = d[i].begin();
				for (auto it = origDS.begin(); it != origDS.end(); it++, DSit++) {
					if (*it == STOUT_INPUT) continue;
					it++;
					if ((it == origDS.end()) || (*it != STOUT_INPUT)) {
						d[i].insert(++DSit, STOUT_INPUT);
						DSit--;
					}
					it--;
				}
				if (d[i].front() == STOUT_INPUT) d[i].pop_front();
			}
			CS.push_back(STOUT_INPUT);// the first symbol
		}

		seq_len_t adsLen = 0;
		for (auto seq : d) {
			if (adsLen < d.size()) {
				adsLen = seq_len_t(d.size());
			}
		}

		shortest_paths_t sp;
		FSMmodel::createAllShortestPaths(fsm, sp);
		sequence_vec_t trSeq(N);
		if (resetEnabled) {
			sequence_set_t sc;
			getStateCover(fsm, sc, fsm->isOutputState());
			for (auto seq : sc) {
				trSeq[fsm->getEndPathState(0, seq)] = seq;
			}
		}

		// create test segments
		state_t counter = 0;
		sequence_vec_t tests;
		for (state_t state = 0; state < N; state++) {
			for (input_t input = 0; input < P; input++) {
				state_t nextState = fsm->getNextState(state, input);
				if (nextState == NULL_STATE) {
					sequence_in_t seq;
					tests.push_back(seq);
					continue;
				}
				sequence_in_t seq(d[nextState]);
				if (fsm->isOutputState()) seq.push_front(STOUT_INPUT);
				seq.push_front(input);
				tests.push_back(seq);
				counter++;
				//printf("%d-%d idx%d len%d %d %s\n", state, input, tests.size() - 1, seq.size(), fsm->getEndPathState(state,seq),
				//    FSMutils::getInSequenceAsString(seq).c_str());
			}
		}
		tests.push_back(d[0]);

		// compute costs
		state_t Tsize = state_t(tests.size());
		seq_len_t maxCost = adsLen + N + 1;
		vector<vector<seq_len_t> > costs(Tsize);
		priority_queue<pq_entry_t, vector<pq_entry_t>, pq_entry_comp> edges;
		for (state_t i = 0; i < Tsize; i++) {
			costs[i].resize(Tsize, maxCost);
		}
		state_t idx = 0;
		for (state_t state = 0; state < N; state++) {
			for (input_t input = 0; input < P; input++, idx++) {
				auto actState = fsm->getNextState(states[state], input);
				if (actState == NULL_STATE) continue;
				actState = getIdx(states, actState);
				//printf("%d %d %s\n", state, input, FSMutils::getInSequenceAsString(tests[idx]).c_str());
				auto it = tests[idx].begin();
				seq_len_t cost = 1;
				for (++it; it != tests[idx].end(); it++, cost++) {
					if (fsm->isOutputState()) {
						it++;
						if (it == tests[idx].end()) break;
					}
					state_t nextIdx = actState * P + (*it);
					//printf("%d-%d %di%d %dx%d c%d %s\n", state, actState, input, *it, idx, nextIdx, costs[idx][nextIdx],
					//       FSMutils::getInSequenceAsString(tests[nextIdx]).c_str());
					if ((costs[idx][nextIdx] == maxCost) && (nextIdx != idx) && !tests[nextIdx].empty()) {
						if (equalSeqPart(it, tests[idx].end(), tests[nextIdx].begin(), tests[nextIdx].end())) {
							costs[idx][nextIdx] = cost;
							pq_entry_t en(idx, nextIdx, cost);
							edges.push(en);
							//printf("%dx%d %d\n", idx,nextIdx, cost);
						}
					}
					actState = fsm->getNextState(actState, *it);
					actState = getIdx(states, actState);
				}
				costs[idx][Tsize - 1] = seq_len_t(tests[idx].size());
				for (state_t i = 0; i < Tsize - 1; i++) {
					if ((costs[idx][i] == maxCost) && (idx != i) && !tests[i].empty()) {
						if (actState == i / P) {// actState = end state of tests[idx]; i/P is start state of tests[i]
							costs[idx][i] = seq_len_t(tests[idx].size());
						}
						else if (resetEnabled && (trSeq[i / P].size() + 1 < sp[actState][i / P].first)) {
							costs[idx][i] = seq_len_t(tests[idx].size() + trSeq[i / P].size() + 1);
						}
						else {
							costs[idx][i] = seq_len_t(tests[idx].size() + sp[actState][i / P].first);
						}
						pq_entry_t en(idx, i, costs[idx][i]);
						edges.push(en);
						//printf("%dx%d %d %d-%d %d\n", idx, i, costs[idx][i], actState, i / P, sp[actState][i / P].first);
					}
				}
			}
		}
		idx = Tsize - 1;
		auto it = tests[idx].begin();
		state_t actState = 0;
		seq_len_t cost = 0;
		for (; it != tests[idx].end(); it++, cost++) {
			state_t nextIdx = actState * P + (*it);
			if ((costs[idx][nextIdx] == maxCost) && !tests[nextIdx].empty()) {
				if (equalSeqPart(it, tests[idx].end(), tests[nextIdx].begin(), tests[nextIdx].end())) {
					costs[idx][nextIdx] = cost;
					pq_entry_t en(idx, nextIdx, cost);
					edges.push(en);
				}
			}
			actState = fsm->getNextState(actState, *it);
			actState = getIdx(states, actState);
			if (fsm->isOutputState()) it++;
		}
		for (state_t i = 0; i < Tsize - 1; i++) {
			if ((costs[idx][i] == maxCost) && !tests[i].empty()) {
				if (actState == i / P) {
					costs[idx][i] = seq_len_t(tests[idx].size());
				}
				else if (resetEnabled && (trSeq[i / P].size() + 1 < sp[actState][i / P].first)) {
					costs[idx][i] = seq_len_t(tests[idx].size() + trSeq[i / P].size() + 1);
				}
				else {
					costs[idx][i] = seq_len_t(tests[idx].size() + sp[actState][i / P].first);
				}
				pq_entry_t en(idx, i, costs[idx][i]);
				edges.push(en);
				//printf("%d %d %d-%d %d %d\n", idx, i, actState, i / P, sp[actState][i / P].first, tests[idx].size());
			}
		}

		// write LP
		mkdir(LP_FOLDER);
		string fileName = fsm->getFilename();
		fileName = FSMlib::Utils::getUniqueName(fileName, "lp", string(LP_FOLDER)+"/");

		if (!writeLP(costs, fileName)) return false;
		string call = "start " + string(gurobiPath) + GUROBI_SOLVER + " ResultFile=" + fileName + ".sol " + fileName;
		//printf(call.c_str());
		int rv = system(call.c_str());
		if (rv != 0) {
			ERROR_MESSAGE("Mstar-method: Fail in solving LP.\n");
			return false;
		}

		// obtain solution of LP
		vector<state_t> next(Tsize);
		getSolution(next, fileName + ".sol");

		// create CS
		idx = Tsize - 1;
		for (state_t i = 0; i < Tsize - 1; i++) {
			//printf("%d %d %d %d %d\n", idx, next[idx], costs[idx][next[idx]], tests[idx].size(), CS.size());
			if (costs[idx][next[idx]] >= tests[idx].size()) {
				CS.insert(CS.end(), tests[idx].begin(), tests[idx].end());
				state_t from = fsm->getEndPathState((idx == Tsize - 1) ? 0 : idx / P, tests[idx]);
				if (from != next[idx] / P) {
					if (resetEnabled && (trSeq[next[idx] / P].size() + 1 < sp[from][next[idx] / P].first)) {
						sequence_in_t seq(CS);
						TS.insert(seq);
						CS.clear();
						CS.assign(trSeq[next[idx] / P].begin(), trSeq[next[idx] / P].end());
					}
					else {
						sequence_in_t shortestPath;
						FSMmodel::getShortestPath(fsm, from, next[idx] / P, sp, shortestPath, fsm->isOutputState());
						//printf(" %d\n", shortestPath.size());
						CS.insert(CS.end(), shortestPath.begin(), shortestPath.end());
					}
				}
			}
			else {
				auto it = tests[idx].begin();
				for (seq_len_t j = 0; j < costs[idx][next[idx]]; j++) {
					it++;
					if (fsm->isOutputState()) it++;
				}
				CS.insert(CS.end(), tests[idx].begin(), it);
			}
			idx = next[idx];
		}
		//printf("%d %d %d %d %d\n", idx, next[idx], costs[idx][next[idx]], tests[idx].size(), CS.size());
		CS.insert(CS.end(), tests[idx].begin(), tests[idx].end());
		//printf("%d %d\n", idx, CS.size());
		TS.insert(CS);
		return true;
	}

	bool Mstar_method(DFSM* fsm, sequence_in_t& CS, int extraStates) {
		sequence_set_t TS;
		CS.clear();
		if (!process_Mstar(fsm, TS, extraStates, false)) return false;
		CS.insert(CS.end(), TS.begin()->begin(), TS.begin()->end());
		return true;
	}

	bool Mrstar_method(DFSM* fsm, sequence_set_t& TS, int extraStates) {
		return process_Mstar(fsm, TS, extraStates, true);
	}

}
