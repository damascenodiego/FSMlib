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

#include "FSMsequence.h"

namespace FSMsequence {
	typedef set<state_t> block_t;
	typedef set<block_t> partition_t;

	struct hs_node_t {
		partition_t partition;
		sequence_in_t hs;

		hs_node_t(partition_t partition, sequence_in_t hs) : 
			partition(partition), hs(hs) {
		}
	};

	static const state_t getSetId(const block_t & block) {
		state_t sum = 0;
		for (auto state : block) {
			sum += state;
		}
		sum += state_t(block.size());
		return sum;
	}

	static bool isSubsetPartition(partition_t::iterator subsetFirstIt, partition_t::iterator subsetLastIt,
		partition_t::iterator partitionFirstIt, partition_t::iterator partitioLastIt) {
		while ((subsetFirstIt != subsetLastIt) && (partitionFirstIt != partitioLastIt)) {
			if (*subsetFirstIt == *partitionFirstIt) {
				subsetFirstIt++;
				partitionFirstIt++;
			}
			else if (*subsetFirstIt > *partitionFirstIt) {
				partitionFirstIt++;
			}
			else {
				return false;
			}
		}
		return (subsetFirstIt == subsetLastIt);
	}

	sequence_in_t getPresetHomingSequence(const unique_ptr<DFSM>& fsm, bool omitUnnecessaryStoutInputs) {
		RETURN_IF_UNREDUCED(fsm, "FSMsequence::getPresetHomingSequence", sequence_in_t());
		sequence_in_t outHS;
		partition_t partition;
		sequence_in_t s;
		if (fsm->isOutputState()) {
			vector<block_t> sameOutput(fsm->getNumberOfOutputs() + 1);// +1 for DEFAULT_OUTPUT
			set<output_t> actOutputs;
			// get output of all states
			for (state_t state = 0; state < fsm->getNumberOfStates(); state++) {
				auto output = fsm->getOutput(state, STOUT_INPUT);
				if (output == DEFAULT_OUTPUT) output = fsm->getNumberOfOutputs();
				sameOutput[output].emplace(state);
				actOutputs.emplace(output);
			}
			if (actOutputs.size() > 1)
				s.push_back(STOUT_INPUT);
			// save block with more then one state and clear sameOutput
			for (auto& out : actOutputs) {
				if (sameOutput[out].size() > 1) {
					partition.emplace(move(sameOutput[out]));
				}
			}
			// all blocks are singletons
			if (partition.empty()) {
				outHS.push_back(STOUT_INPUT);
				return outHS;
			}
		}
		else {
			block_t states;
			for (state_t state = 0; state < fsm->getNumberOfStates(); state++) {
				states.emplace(state);
			}
			partition.emplace(move(states));
		}
		queue<unique_ptr<hs_node_t>> fifo;
		// <id, node's partition>, id = getSetId(node) = sum of state IDs in the first block of node's partition
		multimap<state_t, partition_t> used;
		bool stop, stoutNeeded = false;
		bool useStout = !omitUnnecessaryStoutInputs && fsm->isOutputState();

		fifo.emplace(make_unique<hs_node_t>(partition, s));
		used.emplace(getSetId(*partition.begin()), move(partition));
		while (!fifo.empty()) {
			auto act = move(fifo.front());
			fifo.pop();
			for (input_t input = 0; input < fsm->getNumberOfInputs(); input++) {
				stop = false;
				partition_t partition;
				// go through all blocks in current partition
				for (const auto& block : act->partition) {
					vector<block_t> sameOutput(fsm->getNumberOfOutputs() + 1);// +1 for DEFAULT_OUTPUT
					set<output_t> actOutputs;
					// go through all states in current block
					for (const auto& state : block) {
						auto output = fsm->getOutput(state, input);
						if (output == DEFAULT_OUTPUT) output = fsm->getNumberOfOutputs();
						if (output == WRONG_OUTPUT) {
							// there is no transition so next state is uncertain
							stop = true;
							break;
						}
						sameOutput[output].emplace(fsm->getNextState(state, input));
						actOutputs.emplace(output);
					}
					// save block with more then one state
					for (auto out : actOutputs) {
						if (!stop && (sameOutput[out].size() > 1)) {
							partition.emplace(move(sameOutput[out]));
						}
					}
					if (stop) break;
				}
				if (stop) {// try another input
					continue;
				}
				if (fsm->isOutputState()) {
					partition_t tmp;
					stoutNeeded = false;
					for (const auto& block : partition) {
						vector<block_t> sameOutput(fsm->getNumberOfOutputs() + 1);// +1 for DEFAULT_OUTPUT
						set<output_t> actOutputs;
						for (const auto& state : block) {
							auto output = fsm->getOutput(state, STOUT_INPUT);
							if (output == DEFAULT_OUTPUT) output = fsm->getNumberOfOutputs();
							sameOutput[output].emplace(state);
							actOutputs.emplace(output);
						}
						if (actOutputs.size() > 1) stoutNeeded = true;
						// save block with more then one state and clear sameOutput
						for (auto out : actOutputs) {
							if (sameOutput[out].size() > 1) {
								tmp.emplace(move(sameOutput[out]));
							}
							sameOutput[out].clear();
						}
						actOutputs.clear();
					}
					if (stoutNeeded) {
						partition.swap(tmp);
					}
				}
				// all blocks are singletons
				if (partition.empty()) {
					outHS.swap(act->hs);
					outHS.push_back(input);
					if (stoutNeeded || useStout) outHS.push_back(STOUT_INPUT);
					return outHS;
				}
				// go through all blocks in new partition
				for (partition_t::iterator pIt = partition.begin(); pIt != partition.end(); pIt++) {
					auto usedIt = used.equal_range(getSetId(*pIt));
					for (auto it = usedIt.first; it != usedIt.second; it++) {
						if (isSubsetPartition(it->second.begin(), it->second.end(), pIt, partition.end())) {
							stop = true;
							break;
						}
					}
					if (stop) break;
				}
				// create new node
				if (!stop) {
					s = act->hs;
					s.push_back(input);
					if (stoutNeeded || useStout) s.push_back(STOUT_INPUT);
					fifo.emplace(make_unique<hs_node_t>(partition, s));
					used.emplace(getSetId(*partition.begin()), move(partition));
				}
			}
		}
		return outHS;
	}
}