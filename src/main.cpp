// ESP32-S3 CH9329 emulator: reads CH9329 frames from UART0 (the board's
// "COM" port) and replays them as USB HID keyboard/mouse reports on the
// native USB OTG port.

#include <Arduino.h>

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "ch9329_parser.h"
#include "hid_abs_mouse.h"

static constexpr uint32_t UART_BAUD = 115200;
static constexpr uint8_t CHIP_VERSION = 0x30;  // what a real CH9329 reports (V1.0)

static USBHIDKeyboard keyboard;
static USBHIDMouse relMouse;
static HidAbsMouse absMouse;
static ch9329::Parser parser;

static volatile uint8_t keyboardLeds = 0;
static volatile bool usbMounted = false;

// USBHIDMouse::buttons() is private; reach the same state via press/release.
static void setRelButtons(uint8_t buttons) {
    static uint8_t current = 0;
    if (uint8_t released = current & ~buttons) relMouse.release(released);
    if (uint8_t pressed = buttons & ~current) relMouse.press(pressed);
    current = buttons;
}

static void reply(uint8_t addr, uint8_t cmd, const uint8_t* data, uint8_t len) {
    uint8_t frame[6 + ch9329::MAX_DATA];
    size_t n = ch9329::buildFrame(addr, cmd, data, len, frame);
    Serial.write(frame, n);
}

static void replyStatus(uint8_t addr, uint8_t cmd, uint8_t status) {
    reply(addr, cmd | 0x80, &status, 1);
}

static void handleFrame() {
    const uint8_t addr = parser.addr();
    const uint8_t cmd = parser.cmd();
    const uint8_t len = parser.len();
    const uint8_t* d = parser.data();

    switch (cmd) {
    case ch9329::CMD_GET_INFO: {
        const uint8_t info[8] = {CHIP_VERSION, static_cast<uint8_t>(usbMounted ? 1 : 0), keyboardLeds, 0, 0, 0, 0, 0};
        reply(addr, cmd | 0x80, info, sizeof(info));
        return;
    }
    case ch9329::CMD_SEND_KB_GENERAL_DATA: {
        if (len != 8) break;
        KeyReport report;
        report.modifiers = d[0];
        report.reserved = 0;
        memcpy(report.keys, d + 2, 6);
        keyboard.sendReport(&report);
        replyStatus(addr, cmd, ch9329::STATUS_SUCCESS);
        return;
    }
    case ch9329::CMD_SEND_MS_ABS_DATA: {
        // 0x02, buttons, xL, xH, yL, yH, wheel
        if (len != 7 || d[0] != 0x02) break;
        uint16_t x = d[2] | (d[3] << 8);
        uint16_t y = d[4] | (d[5] << 8);
        absMouse.send(d[1], x, y, static_cast<int8_t>(d[6]));
        replyStatus(addr, cmd, ch9329::STATUS_SUCCESS);
        return;
    }
    case ch9329::CMD_SEND_MS_REL_DATA: {
        // 0x01, buttons, dx, dy, wheel
        if (len != 5 || d[0] != 0x01) break;
        setRelButtons(d[1] & 0x1F);
        int8_t dx = static_cast<int8_t>(d[2]);
        int8_t dy = static_cast<int8_t>(d[3]);
        int8_t wheel = static_cast<int8_t>(d[4]);
        if (dx || dy || wheel) relMouse.move(dx, dy, wheel);
        replyStatus(addr, cmd, ch9329::STATUS_SUCCESS);
        return;
    }
    default:
        replyStatus(addr, cmd, ch9329::STATUS_ERR_CMD);
        return;
    }
    replyStatus(addr, cmd, ch9329::STATUS_ERR_PARAM);
}

static void onUsbEvent(void*, esp_event_base_t base, int32_t id, void*) {
    if (base != ARDUINO_USB_EVENTS) return;
    if (id == ARDUINO_USB_STARTED_EVENT || id == ARDUINO_USB_RESUME_EVENT) usbMounted = true;
    if (id == ARDUINO_USB_STOPPED_EVENT || id == ARDUINO_USB_SUSPEND_EVENT) usbMounted = false;
}

static void onKeyboardEvent(void*, esp_event_base_t base, int32_t id, void* data) {
    if (base == ARDUINO_USB_HID_KEYBOARD_EVENTS && id == ARDUINO_USB_HID_KEYBOARD_LED_EVENT) {
        keyboardLeds = static_cast<arduino_usb_hid_keyboard_event_data_t*>(data)->leds;
    }
}

void setup() {
    Serial.setRxBufferSize(1024);
    Serial.begin(UART_BAUD);

    USB.VID(0x303A);
    USB.PID(0x8129);
    USB.manufacturerName("makvm");
    USB.productName("ESP32-S3 CH9329 KVM");
    USB.onEvent(onUsbEvent);
    keyboard.onEvent(onKeyboardEvent);

    keyboard.begin();
    relMouse.begin();
    absMouse.begin();
    USB.begin();
}

void loop() {
    while (Serial.available()) {
        if (parser.feed(static_cast<uint8_t>(Serial.read()))) handleFrame();
    }
    delay(1);
}
