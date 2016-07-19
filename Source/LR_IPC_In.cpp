/*
  ==============================================================================

    LR_IPC_In.cpp

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
#include "LR_IPC_In.h"
#include "Utilities/Utilities.h"
#include <bitset>
#include <stdexcept>

namespace {
  constexpr auto kBufferSize = 256;
  constexpr auto kConnectTryTime = 100;
  constexpr auto kEmptyWait = 100;
  constexpr auto kLrInPort = 58764;
  constexpr auto kMaxMIDI = 127.0;
  constexpr auto kMaxNRPN = 16383.0;
  constexpr auto kNotConnectedWait = 333;
  constexpr auto kReadyWait = 0;
}

LR_IPC_IN::LR_IPC_IN(): juce::StreamingSocket{}, juce::Thread{"LR_IPC_IN"} {}

LR_IPC_IN::~LR_IPC_IN() {
  {
    std::lock_guard<decltype(timer_mutex_)> lock(timer_mutex_);
    timer_off_ = true;
    juce::Timer::stopTimer();
  }
  juce::Thread::stopThread(1000);
  juce::StreamingSocket::close();
}

void LR_IPC_IN::Init(std::shared_ptr<CommandMap>& map_command,
  std::shared_ptr<ProfileManager>& profile_manager,
  std::shared_ptr<MIDISender>& midi_sender) noexcept {
  command_map_ = map_command;
  profile_manager_ = profile_manager;
  midi_sender_ = midi_sender;
  //start the timer
  juce::Timer::startTimer(1000);
}

void LR_IPC_IN::refreshMIDIOutput() {
  if (command_map_ && midi_sender_) {
      // send associated CC messages to MIDI OUT devices
    for (const auto& map_entry : parameter_map_) {
      if (command_map_->commandHasAssociatedMessage(map_entry.first)) {
        const auto& msg = command_map_->getMessageForCommand(map_entry.first);
        const auto value = static_cast<int>(round(
          ((msg.controller < 128) ? kMaxMIDI : kMaxNRPN) * map_entry.second));
        midi_sender_->sendCC(msg.channel, msg.controller, value);
      }
    }
  }
}

void LR_IPC_IN::PleaseStopThread() {
  juce::Thread::signalThreadShouldExit();
  juce::Thread::notify();
}

void LR_IPC_IN::run() {
  while (!juce::Thread::threadShouldExit()) {
    //if not connected, executes a wait 333 then goes back to while
    //if connected, tries to read a line, checks thread status and connection
    //status before each read attempt
    //doesn't terminate thread if disconnected, as currently don't have graceful
    //way to restart thread
    if (!juce::StreamingSocket::isConnected()) {
      juce::Thread::wait(kNotConnectedWait);
    } //end if (is not connected)
    else {
      char line[kBufferSize + 1] = {'\0'};//plus one for \0 at end
      auto size_read = 0;
      auto can_read_line = true;
      // parse input until we have a line, then process that line, quit if
      // connection lost
      while (std::string(line).back() != '\n' && juce::StreamingSocket::isConnected()) {
        if (juce::Thread::threadShouldExit())
          goto threadExit;//break out of nested whiles
        const auto wait_status = juce::StreamingSocket::waitUntilReady(true, kReadyWait);
        switch (wait_status) {
          case -1:
            can_read_line = false;
            goto dumpLine; //read line failed, break out of switch and while
          case 0:
            juce::Thread::wait(kEmptyWait);
            break; //try again to read until char shows up
          case 1:
            if (size_read == kBufferSize)
              throw std::out_of_range("Buffer overflow in LR_IPC_IN");
            size_read += juce::StreamingSocket::read(line + size_read, 1, false);
            break;
        }
      } // end while !\n and is connected

      // if lose connection, line may not be terminated
      {
        std::string param{line};
        if (can_read_line && param.back() == '\n') {
          processLine(param);
        }
      } //scope param
    dumpLine: /* empty statement */;
    } //end else (is connected)
  } //while not threadshouldexit
threadExit: /* empty statement */;
  std::lock_guard<decltype(timer_mutex_)> lock(timer_mutex_);
  timer_off_ = true;
  juce::Timer::stopTimer();
  //thread_started_ = false; //don't change flag while depending upon it
}

void LR_IPC_IN::timerCallback() {
  std::lock_guard<decltype(timer_mutex_)> lock(timer_mutex_);
  if (!juce::StreamingSocket::isConnected() && !timer_off_) {
    if (juce::StreamingSocket::connect("127.0.0.1", kLrInPort, kConnectTryTime))
      if (!thread_started_) {
        juce::Thread::startThread(); //avoid starting thread during shutdown
        thread_started_ = true;
      }
  }
}
void LR_IPC_IN::processLine(const std::string& line) {
  const static std::unordered_map<std::string, int> cmds = {
    {"SwitchProfile",1},
    {"SendKey",2},
    {"TerminateApplication",3},
  };
    // process input into [parameter] [Value]
  const auto trimmed_line = RSJ::trim(line);
  const auto command = trimmed_line.substr(0, trimmed_line.find(' '));
  const auto value_string = trimmed_line.substr(trimmed_line.find(' ') + 1);

  switch (cmds.count(command) ? cmds.at(command) : 0) {
    case 1: //SwitchProfile
      if (profile_manager_)
        profile_manager_->switchToProfile(value_string);
      break;
    case 2: //SendKey
      {
        std::bitset<3> modifiers{static_cast<decltype(modifiers)>
          (std::stoi(value_string))};
        send_keys_.SendKeyDownUp(RSJ::ltrim(RSJ::ltrim(value_string, RSJ::digit)),
          modifiers[0], modifiers[1], modifiers[2]);
        break;
      }
    case 3: //TerminateApplication
      PleaseStopThread();
      JUCEApplication::getInstance()->systemRequestedQuit();
      break;
    case 0:
      // store updates in map
      const auto original_value = std::stod(value_string);
      parameter_map_[command] = original_value;
      // send associated CC messages to MIDI OUT devices
      if (command_map_ && command_map_->commandHasAssociatedMessage(command)) {
        const auto& msg = command_map_->getMessageForCommand(command);
        const auto value = static_cast<int>(round(
          ((msg.controller < 128) ? kMaxMIDI : kMaxNRPN) * original_value));
        if (midi_sender_) {
          switch (msg.messageType) {
            case MessageType::NOTE: midi_sender_->sendCC(msg.channel, msg.controller, value); break;
            case MessageType::CC: midi_sender_->sendCC(msg.channel, msg.controller, value); break;
            case MessageType::PITCHBEND: midi_sender_->sendPitchBend(msg.channel, static_cast<int>(round(original_value * 15300.0))); break;
          }
        }
      }
  }
}