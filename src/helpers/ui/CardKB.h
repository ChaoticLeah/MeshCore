#pragma once

#ifdef HAS_CARDKB
#include <Arduino.h>
#include <Wire.h>

#define CARDKB_ADDR 0x5F

/*
 * M5Stack CardKB I2C Keyboard Driver
 *
 * Simple I2C keyboard interface for the M5Stack CardKB
 * Features:
 * - Full QWERTY keyboard input
 * - Returns ASCII characters
 * - Special keys: 0x08 (backspace), 0x0D (enter), 0x1B (escape)
 * - Non-blocking polling
 * - I2C address: 0x5F
 */

class CardKB {
public:
	void begin(TwoWire *wire = &Wire);
	char getKey();           // Returns '\0' if no key pressed
	bool available();        // Check if a key is available
	const char* getDebugStatus() { return _debug_status; }
	uint32_t getReadCount() { return _read_count; }
	uint32_t getErrorCount() { return _error_count; }
	uint32_t getKeyCount() { return _key_count; }
  
private:
	TwoWire *_wire;
	char _rx_buffer;         // Single byte RX buffer for TWIM
	char _debug_status[32];  // Debug status string
	uint32_t _read_count;
	uint32_t _error_count;
	uint32_t _key_count;
};

#endif
