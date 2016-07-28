#pragma once
/*
  ==============================================================================

    LRCommands.h

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
#ifndef LRCOMMANDS_H_INCLUDED
#define LRCOMMANDS_H_INCLUDED
#include <map>
#include <string>
#include <vector>
#include "../JuceLibraryCode/JuceHeader.h"

class LRCommandList {
public:
  const std::string& getCommandByIndex(size_t idx);
  size_t getIndexOfCommand(const std::string& command);
  size_t getNumberOfCommands();
  std::vector<std::string> GetHeadings() const;
  std::vector<std::vector<std::string>> GetMenuStructure() const;
  LRCommandList();
private:
  std::vector<std::string> headings;
  std::vector<std::vector<std::string>> menu_structure;
  std::map<std::string, size_t> menu_order;
  std::vector<std::string> command_by_number;
  std::map<std::string, std::string> cmd_to_name;
  std::map<std::string, std::string> name_to_cmd;
  void ReadInStrings();
};

inline  std::vector<std::string> LRCommandList::GetHeadings() const {
  return headings;
}

inline std::vector<std::vector<std::string>> LRCommandList::GetMenuStructure() const {
  return menu_structure;
}

inline size_t LRCommandList::getIndexOfCommand(const std::string& command) {
  return menu_order[command];
}

inline const std::string& LRCommandList::getCommandByIndex(size_t idx) {
  return command_by_number[idx];
}

inline size_t LRCommandList::getNumberOfCommands() {
  return command_by_number.size();
}

#endif  // LRCOMMANDS_H_INCLUDED
