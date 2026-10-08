#include "hid_abs_mouse.h"

#include "ch9329_parser.h"

// clang-format off
static const uint8_t report_descriptor[] = {
    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x02,                    // Usage (Mouse)
    0xA1, 0x01,                    // Collection (Application)
    0x85, HidAbsMouse::REPORT_ID,  //   Report ID
    0x09, 0x01,                    //   Usage (Pointer)
    0xA1, 0x00,                    //   Collection (Physical)
    0x05, 0x09,                    //     Usage Page (Button)
    0x19, 0x01,                    //     Usage Minimum (1)
    0x29, 0x05,                    //     Usage Maximum (5)
    0x15, 0x00,                    //     Logical Minimum (0)
    0x25, 0x01,                    //     Logical Maximum (1)
    0x75, 0x01,                    //     Report Size (1)
    0x95, 0x05,                    //     Report Count (5)
    0x81, 0x02,                    //     Input (Data, Var, Abs)
    0x75, 0x03,                    //     Report Size (3)
    0x95, 0x01,                    //     Report Count (1)
    0x81, 0x03,                    //     Input (Const) - padding
    0x05, 0x01,                    //     Usage Page (Generic Desktop)
    0x09, 0x30,                    //     Usage (X)
    0x09, 0x31,                    //     Usage (Y)
    0x15, 0x00,                    //     Logical Minimum (0)
    0x26, 0xFF, 0x0F,              //     Logical Maximum (4095)
    0x75, 0x10,                    //     Report Size (16)
    0x95, 0x02,                    //     Report Count (2)
    0x81, 0x02,                    //     Input (Data, Var, Abs)
    0x09, 0x38,                    //     Usage (Wheel)
    0x15, 0x81,                    //     Logical Minimum (-127)
    0x25, 0x7F,                    //     Logical Maximum (127)
    0x75, 0x08,                    //     Report Size (8)
    0x95, 0x01,                    //     Report Count (1)
    0x81, 0x06,                    //     Input (Data, Var, Rel)
    0xC0,                          //   End Collection
    0xC0,                          // End Collection
};
// clang-format on

struct __attribute__((packed)) AbsMouseReport {
    uint8_t buttons;
    uint16_t x;
    uint16_t y;
    int8_t wheel;
};

HidAbsMouse::HidAbsMouse() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        USBHID::addDevice(this, sizeof(report_descriptor));
    }
}

void HidAbsMouse::begin() { hid_.begin(); }

uint16_t HidAbsMouse::_onGetDescriptor(uint8_t* dst) {
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

bool HidAbsMouse::send(uint8_t buttons, uint16_t x, uint16_t y, int8_t wheel) {
    AbsMouseReport report = {
        static_cast<uint8_t>(buttons & 0x1F),
        ch9329::clampAbs(x),
        ch9329::clampAbs(y),
        wheel,
    };
    return hid_.SendReport(REPORT_ID, &report, sizeof(report));
}
