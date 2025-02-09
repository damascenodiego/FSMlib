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

#include "FSMtesting.h"
#include "../PrefixSet.h"

using namespace FSMsequence;

namespace FSMtesting {
	sequence_set_t Wp_method(const unique_ptr<DFSM>& fsm, int extraStates, const vector<sequence_set_t>& H) {
		RETURN_IF_UNREDUCED(fsm, "FSMtesting::Wp_method", sequence_set_t());
		if (extraStates < 0) {
			return sequence_set_t();
		}
		auto stateCover = getStateCover(fsm);
		auto traversalSet = getTraversalSet(fsm, extraStates);
		traversalSet.emplace(sequence_in_t());
		vector<sequence_set_t> SCSetsTmp, &SCSets(SCSetsTmp);
		if (H.empty()) SCSetsTmp = getStatesCharacterizingSets(fsm, getStatePairsShortestSeparatingSequences, true, reduceSCSet_LS_SL);
		else SCSets = H;
		bool startWithStout = (SCSets[0].begin()->front() == STOUT_INPUT);
		FSMlib::PrefixSet pset;

		if (fsm->isOutputState()) {
			extraStates *= 2; // STOUT_INPUT follows each input in traversalSet
		}
		// CSet design
		for (const auto& SCSet : SCSets) {
			for (const auto& seq : SCSet) {
				pset.insert(seq);
			}
		}
		auto CSet = pset.getMaximalSequences();
		pset.clear();
	
		for (const auto& trSeq : stateCover) {
			for (const auto& extSeq : traversalSet) {
				sequence_in_t transferSeq(trSeq);
				transferSeq.insert(transferSeq.end(), extSeq.begin(), extSeq.end());
				state_t state = fsm->getEndPathState(0, transferSeq);
				if (state == WRONG_STATE) continue;
				for (const auto& cSeq : CSet) {
					sequence_in_t testSeq(transferSeq);
					if (startWithStout) {
						testSeq.push_front(STOUT_INPUT);
						testSeq.pop_back();// the last STOUT_INPUT (it will be at the beginning of appended cSeq)
					}
					testSeq.insert(testSeq.end(), cSeq.begin(), cSeq.end());
					pset.insert(move(testSeq));
				}
				if (extSeq.size() == extraStates) {// check outcoming transitions
					//state_t state = getIdx(states, fsm->getEndPathState(0, transferSeq));
					for (input_t input = 0; input < fsm->getNumberOfInputs(); input++) {
						// SCSet is sufficient for transition verification
						state_t nextState = fsm->getNextState(state, input);
						if (nextState == NULL_STATE) {
							sequence_in_t testSeq(transferSeq);
							testSeq.push_back(input);
							if (startWithStout) {
								testSeq.push_front(STOUT_INPUT);
							}
							pset.insert(move(testSeq)); 
							continue;
						}
						for (const auto& cSeq : SCSets[nextState]) {
							sequence_in_t testSeq(transferSeq);
							testSeq.push_back(input);
							if (startWithStout) {
								testSeq.push_front(STOUT_INPUT);
							}
							else if (fsm->isOutputState()) {
								testSeq.push_back(STOUT_INPUT);
							}
							testSeq.insert(testSeq.end(), cSeq.begin(), cSeq.end());
							pset.insert(move(testSeq));
						}
					}
				}
			}
		}
		return pset.getMaximalSequences();
	}
}
