#pragma once

#ifdef HAS_CARDKB
#include "UITask.h"
#include <helpers/ui/CardKB.h>

class ComposeScreen : public UIScreen {
  UITask* _task;
  CardKB* _keyboard;
  char _message_buffer[256];
  uint16_t _cursor_pos;
  uint8_t _selected_channel;
  // Array remapping (since channels in the original array get random indexes and we need to fix this)
  uint8_t _valid_channel_indices[MAX_GROUP_CHANNELS];
  uint8_t _valid_channel_count;

  private:
    void rebuildChannelIndex() {
      _valid_channel_count = 0;

      for (uint8_t ch = 0; ch < MAX_GROUP_CHANNELS; ++ch) {
        ChannelDetails channel;
        the_mesh.getChannel(ch, channel);
        if (channel.name[0] != '\0') {
          _valid_channel_indices[_valid_channel_count++] = ch;
        }
      }
    }

    uint8_t getRealSelectedChannel() const {
      if (_selected_channel < _valid_channel_count) {
        return _valid_channel_indices[_selected_channel];
      }
      return 0;  // default to channel 0
    }

public:
  ComposeScreen(UITask* task, CardKB* keyboard) 
    : _task(task), _keyboard(keyboard), _cursor_pos(0), _selected_channel(0), _valid_channel_count(0) {
    _message_buffer[0] = '\0';
  }

  void reset() {
    _cursor_pos = 0;
    _message_buffer[0] = '\0';
    _selected_channel = 0;
    rebuildChannelIndex();
  }

  int render(DisplayDriver& display) override {
    display.setTextSize(1);
    display.setColor(DisplayDriver::YELLOW);
    display.setCursor(0, 0);
    display.print("Compose Message");

    // Draw selected channel
    ChannelDetails channel;
    display.setColor(DisplayDriver::GREEN);
    display.setCursor(0, 15);
    if (the_mesh.getChannel(getRealSelectedChannel(), channel)) {
      display.print("Channel: ");
      display.print(channel.name);
    } else {
      display.print("Channel: (none)");
    }
    display.print(" (Up/Down)");

    // Draw message buffer
    display.setColor(DisplayDriver::LIGHT);
    display.setCursor(0, 30);
    display.print(_message_buffer);
    
    // Draw cursor
    if ((millis() / 500) % 2 == 0) {
      display.fillRect(display.getTextWidth(_message_buffer) + 2, 30, 2, 12);
    }

    // Instructions at bottom
    display.setColor(DisplayDriver::GREEN);
    display.setTextSize(1);
    display.setCursor(0, display.height() - 8);
    display.print("Press ENTER to send");

    return 100;  // refresh every 100ms for cursor blink
  }

  bool handleInput(char c) override {
    if (c == KEY_UP) {
      if (_selected_channel > 0) {
        _selected_channel--;
      }
      return true;
    } else if (c == KEY_DOWN) {
      if (_selected_channel + 1 < _valid_channel_count) {
        _selected_channel++;
      }
      return true;
    } else if (c == KEY_ENTER || c == KEY_SELECT) {
      if (_cursor_pos > 0) {
        _message_buffer[_cursor_pos] = '\0';
        ChannelDetails channel;
        if (the_mesh.getChannel(getRealSelectedChannel(), channel)) {
          uint32_t timestamp = the_mesh.getRTCClock()->getCurrentTime();
          const char* node_name = the_mesh.getNodeName();
          if (the_mesh.sendGroupMessage(timestamp, channel.channel, node_name, _message_buffer, _cursor_pos)) {
            the_mesh.onSelfChannelMessage(channel.channel, timestamp, _message_buffer);
            _task->showAlert("Message Sent!", 1500);
            reset();
          } else {
            _task->showAlert("Send Failed!", 1500);
          }
        } else {
          _task->showAlert("No Channel!", 1500);
        }  
        _task->gotoHomeScreen();
      }
      return true;
    } else if (c == 0x08) {  // Backspace
      if (_cursor_pos > 0) {
        _cursor_pos--;
        _message_buffer[_cursor_pos] = '\0';
      }
      return true;
    } else if (c == 0x1B) { // Escape
      _task->gotoHomeScreen();
      return true;
    } else if (c >= 0x20 && c < 0x7F) {  // Printable ASCII
      if (_cursor_pos < sizeof(_message_buffer) - 1) {
        _message_buffer[_cursor_pos++] = c;
        _message_buffer[_cursor_pos] = '\0';
      }
      return true;
    }
    return false;
  }

  void processKeyboardInput() {
    if (_keyboard == NULL) return;
    
    char key = _keyboard->getKey();
    if (key != '\0') {
      handleInput(key);
    }
  }

  void poll() override {
    processKeyboardInput();
  }
};

#endif