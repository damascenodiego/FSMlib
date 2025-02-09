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
#pragma once

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <queue>
#include <stack>
#include <memory>
#include <functional>
#include <time.h>
#include <algorithm>

#include "../FSMlib.h"

using namespace std;

typedef unsigned int machine_type_t;

static const char* machineTypeNames[] = {
	"Invalid",
#define TYPE_NONE       0
	"DFSM",
#define TYPE_DFSM		1
	"Mealy",
#define TYPE_MEALY      2
	"Moore",
#define TYPE_MOORE      3
	"DFA",
#define TYPE_DFA		4
	"NFA",
#define TYPE_NFA		5
	"NFAeps",
#define TYPE_NFAeps		6
	"NFSM",
#define TYPE_NFSM		7
	"NMealy",
#define TYPE_NMEALY		8
	"NMoore",
#define TYPE_NMOORE		9
	"NFSMeps",
#define TYPE_NFSMeps	10
};

// an input invoking the output of a state
#define STOUT_INPUT     input_t(-1)
#define STOUT_SYMBOL	"S"
#define EPSILON_INPUT	input_t(-2)
#define EPSILON_SYMBOL	"E"

#define DEFAULT_OUTPUT	output_t(-1)
#define DEFAULT_OUTPUT_SYMBOL	"-"
#define WRONG_OUTPUT	output_t(-2)
#define WRONG_OUTPUT_SYMBOL		"X"

#define NULL_STATE		state_t(-1)
#define WRONG_STATE		state_t(-2)

typedef unsigned int state_t;
typedef unsigned long input_t;
typedef unsigned int output_t;
typedef size_t seq_len_t;

typedef list<input_t> sequence_in_t;
typedef list<output_t> sequence_out_t;

typedef vector<vector<pair<seq_len_t, input_t>>> shortest_paths_t;
