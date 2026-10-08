#pragma once

#include "USBHID.h"

// Absolute-positioning mouse (tablet-style) with a 0..4095 coordinate space,
// matching the CH9329's absolute mouse.
class HidAbsMouse : public USBHIDDevice {
public:
    static constexpr uint8_t REPORT_ID = HID_REPORT_ID_VENDOR + 1;

    HidAbsMouse();
    void begin();
    bool send(uint8_t buttons, uint16_t x, uint16_t y, int8_t wheel);

    uint16_t _onGetDescriptor(uint8_t* buffer) override;

private:
    USBHID hid_;
};
