/*
  ==============================================================================

    LRCommands.cpp

This file is part of MIDI2LR. Copyright 2015-2016 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
  ==============================================================================
*/
#include "LRCommands.h"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include "CommandMap.h"

void LRCommandList::ReadInStrings() {
  std::ifstream input("D:\\Adobe Data\\Lightroom Plugins\\MIDI2LR.lrplugin\\AppStrings.txt");
  if (!input.is_open()) {
    input.open("D:\\Adobe Data\\Lightroom Plugins\\MIDI2LR.lrplugin\\AppStringsDefault.txt");
    if (!input.is_open())
      throw std::runtime_error("Translation files not found.");
  }

  for (std::string line; getline(input, line);) {
    const auto t1 = line.find('\t');
    const auto t2 = line.rfind('\t');
    const auto string0 = line.substr(0, t1);
    auto string1 = line.substr(t1 + 1, t2 - t1 - 1);
    auto string2 = line.substr(t2 + 1);
    cmd_to_name[string0] = string2;
    name_to_cmd[string2] = string0;
    bool inserted = false;
    for (auto it = headings.begin(); it != headings.end(); it++) {
      if (*it == string1) {
        inserted = true;
        menu_structure[std::distance(headings.begin(), it)].emplace_back(std::move(string2));
        break;
      }
    }
    if (!inserted) {
      headings.emplace_back(std::move(string1));
      menu_structure.emplace_back(std::vector<std::string>{std::move(string2)});
    }
  }
  size_t ordernum = 0;
  for (const auto& v : menu_structure)
    for (const auto& n : v) {
      const auto cmd = name_to_cmd[n];
      menu_order.emplace(std::make_pair(cmd, ++ordernum));
      command_by_number.push_back(cmd);
    }
}

LRCommandList::LRCommandList() {
  ReadInStrings();
}


