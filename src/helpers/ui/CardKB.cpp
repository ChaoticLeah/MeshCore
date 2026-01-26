#ifdef HAS_CARDKB
#include "CardKB.h"
#include <nrf.h>

// Direct TWIM access for NRF52 with timeout protection
static NRF_TWIM_Type* _twim_instance = nullptr;

void CardKB::begin(TwoWire *wire) {
	_wire = wire;
	_read_count = 0;
	_error_count = 0;
	_key_count = 0;
	strcpy(_debug_status, "Init");
	
	// Arduino Wire uses TWIM1 on NRF52840
	_twim_instance = NRF_TWIM1;
	
	// Enable TWIM if not already enabled
	if ((_twim_instance->ENABLE & TWIM_ENABLE_ENABLE_Msk) == 0) {
		_twim_instance->ENABLE = (TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos);
		strcpy(_debug_status, "TWIM Enabled");
	} else {
		strcpy(_debug_status, "TWIM Active");
	}
}

char CardKB::getKey() {
  if (_wire == NULL || _twim_instance == NULL) {
    strcpy(_debug_status, "Not init");
    return '\0';
  }
  
  // Throttle reads to avoid bus flooding
  static unsigned long last_read = 0;
  unsigned long now = millis();
  if (now - last_read < 30) {
    return '\0';
  }
  last_read = now;
  
  _read_count++;
  
  // Direct TWIM read with timeout to avoid infinite loops
  _twim_instance->ADDRESS = CARDKB_ADDR;
  _twim_instance->RXD.PTR = (uint32_t)&_rx_buffer;
  _twim_instance->RXD.MAXCNT = 1;
  
  // Enable shortcuts for automatic stop after RX
  _twim_instance->SHORTS = TWIM_SHORTS_LASTRX_STOP_Msk;
  
  // Clear events
  _twim_instance->EVENTS_STOPPED = 0;
  _twim_instance->EVENTS_ERROR = 0;
  _twim_instance->EVENTS_RXSTARTED = 0;
  _twim_instance->EVENTS_LASTRX = 0;
  
  // Clear error source
  _twim_instance->ERRORSRC = 0xFFFFFFFF;
  
  // Start RX task
  _twim_instance->TASKS_STARTRX = 1;
  
  // Wait with timeout (max 10ms)
  uint32_t timeout = millis() + 10;
  while (!_twim_instance->EVENTS_STOPPED && 
         !_twim_instance->EVENTS_ERROR &&
         millis() < timeout) {
    // Busy wait with timeout
  }
  
  // Check for errors or timeout
  if (_twim_instance->EVENTS_ERROR) {
    _error_count++;
    uint32_t err = _twim_instance->ERRORSRC;
    snprintf(_debug_status, sizeof(_debug_status), "Err:0x%lX", err);
    _twim_instance->TASKS_STOP = 1;
    timeout = millis() + 5;
    while (!_twim_instance->EVENTS_STOPPED && millis() < timeout) {}
    return '\0';
  }
  
  if (millis() >= timeout) {
    _error_count++;
    strcpy(_debug_status, "Timeout");
    _twim_instance->TASKS_STOP = 1;
    timeout = millis() + 5;
    while (!_twim_instance->EVENTS_STOPPED && millis() < timeout) {}
    return '\0';
  }
  
  // Read succeeded - check if we got data
  if (_twim_instance->RXD.AMOUNT == 1) {
    if (_rx_buffer != 0x00) {
      _key_count++;
      snprintf(_debug_status, sizeof(_debug_status), "Key:0x%02X", (uint8_t)_rx_buffer);
      return _rx_buffer;
    } else {
      snprintf(_debug_status, sizeof(_debug_status), "OK r:%lu", _read_count);
    }
  } else {
    strcpy(_debug_status, "NoData");
  }
  
  return '\0';
}

bool CardKB::available() {
	char c = getKey();
	return (c != '\0');
}

#endif
