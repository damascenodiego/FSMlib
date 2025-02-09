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

#include <iostream>
#include <fstream>
#include <sstream>
#include "FSMlib.h"

using namespace std;

namespace FSMlib {
	void displayErrorMsgOnCerr(const char* errorMsg) {
		std::cerr << errorMsg << std::endl;
	}

	void noticeListeners(const char* msg) {
		(*FSMlib::errorMsgHandler)(msg);
	}

	void setErrorMsgHandler(void(*userErrorMsgHandler)(const char*)) {
		FSMlib::errorMsgHandler = userErrorMsgHandler;
	}

	namespace Utils {
		string hashCode(int length) {
			string hash = "";
			char c;
			for (int i = 0; i < length; i++) {
				do {
					c = char((rand() % 75) + 48);
				} while (!isalnum(c));
				hash += c;
			}
			return hash;
		}

		string getUniqueName(string name, string suffix, string path) {
			string newName = path + name + "." + suffix;
			ifstream file(newName.c_str());
			while (file.is_open()) {
				newName = path + name + "_" + hashCode(5) + "." + suffix;
				file.close();
				file.open(newName.c_str());
			}
			return newName;
		}

		string toString(int number) {
			ostringstream str;
			str << number;
			return str.str();
		}
	}
}
