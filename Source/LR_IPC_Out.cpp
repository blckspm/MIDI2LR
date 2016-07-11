/*
  ==============================================================================

    LR_IPC_OUT.cpp

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
#include "LR_IPC_OUT.h"
#include "CommandMap.h"
#include "LRCommands.h"

namespace {
  constexpr auto kReadyWait = 0; //get out of read quickly
  constexpr auto kConnectTryTime = 100;
  constexpr auto kLrOutPort = 58763;
  constexpr auto kMaxMIDI = 127.0;
  constexpr auto kMaxNRPN = 16383.0;
}

LR_IPC_OUT::LR_IPC_OUT(): StreamingSocket() {}

LR_IPC_OUT::~LR_IPC_OUT() {
  {
    std::lock_guard<decltype(timer_mutex_)> lock(timer_mutex_);
    timer_off_ = true;
    stopTimer();
  }
  close();
  command_map_.reset();
}

void LR_IPC_OUT::Init(std::shared_ptr<CommandMap>& command_map,
  std::shared_ptr<MIDIProcessor>& midi_processor) {
    //copy the pointer
  command_map_ = command_map;

  if (midi_processor) {
    midi_processor->addMIDICommandListener(this);
  }

  //start the timer
  startTimer(1000);
}

void LR_IPC_OUT::addListener(LRConnectionListener *listener) {
  if (std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end())
    listeners_.push_back(listener); //add if not already in list
}

void LR_IPC_OUT::sendCommand(const String &command) {
  {
    std::lock_guard<decltype(command_mutex_)> lock(command_mutex_);
    command_ += command;
  }
  triggerAsyncUpdate();
}

void LR_IPC_OUT::handleMidiCC(int midi_channel, int controller, int value) {
  MIDI_Message message{midi_channel, controller, true};

  if (command_map_) {
    if (!command_map_->messageExistsInMap(message) ||
      command_map_->getCommandforMessage(message) == "Unmapped" ||
      find(LRCommandList::NextPrevProfile.begin(),
        LRCommandList::NextPrevProfile.end(),
        command_map_->getCommandforMessage(message)) != LRCommandList::NextPrevProfile.end())
      return;

    auto command_to_send = command_map_->getCommandforMessage(message);
    double computed_value = value;
    computed_value /= (controller < 128) ? kMaxMIDI : kMaxNRPN;

    command_to_send += String::formatted(" %g\n", computed_value);
    {
      std::lock_guard<decltype(command_mutex_)> lock(command_mutex_);
      command_ += command_to_send;
    }
    triggerAsyncUpdate();
  }
}

void LR_IPC_OUT::handleMidiNote(int midi_channel, int note) {
  MIDI_Message message{midi_channel, note, false};

  if (command_map_) {
    if (!command_map_->messageExistsInMap(message) ||
      command_map_->getCommandforMessage(message) == "Unmapped" ||
      find(LRCommandList::NextPrevProfile.begin(),
        LRCommandList::NextPrevProfile.end(),
        command_map_->getCommandforMessage(message)) != LRCommandList::NextPrevProfile.end())
      return;

    auto command_to_send = command_map_->getCommandforMessage(message);
    command_to_send += String(" 1\n");
    {
      std::lock_guard<decltype(command_mutex_)> lock(command_mutex_);
      command_ += command_to_send;
    }
    triggerAsyncUpdate();
  }
}

void LR_IPC_OUT::ConnectionLost() {
  is_connected_ = false;
  for (auto listener : listeners_)
    listener->disconnected();
}

void LR_IPC_OUT::ConnectionMade() {
  is_connected_ = true;
  for (auto listener : listeners_)
    listener->connected();
}

void LR_IPC_OUT::handleAsyncUpdate() {
  std::lock_guard<decltype(write_mutex_)> updatelock(write_mutex_);
  String command_copy;
  {
    std::lock_guard<decltype(command_mutex_)> lock(command_mutex_);
    command_copy = std::move(command_); //JUCE::String swaps in this case
  }
    //check if there is a connection
  if (isConnected())
    if (waitUntilReady(false, kReadyWait) == 1)
      if (write(command_copy.getCharPointer(), command_copy.length()) > -1)
        return; //success, exit now
  {
    std::lock_guard<decltype(command_mutex_)> lock(command_mutex_);
    command_ = command_copy + command_; //send failed, add back to command list
  }
  triggerAsyncUpdate(); // need to go back in again
}

void LR_IPC_OUT::timerCallback() {
  {
    std::lock_guard<decltype(timer_mutex_)> lock(timer_mutex_);
    if (!timer_off_ && !isConnected() && (++seconds_disconnected_ > kReconnectDelay))
      connect("127.0.0.1", kLrOutPort, kConnectTryTime);
    else
      seconds_disconnected_ = 0;
  }
  //don't care if following occurs after stopping timer, so mutex unnecessary
  if (isConnected()) {
    if (!is_connected_)
      ConnectionMade();
  }
  else if (is_connected_)
    ConnectionLost();
}