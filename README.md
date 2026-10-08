# esp32s3-ch9329

Firmware for an ESP32-S3 dev board (tested: ESP32-S3-N16R8 with two USB-C ports) that emulates a
WCH **CH9329** UART-to-USB-HID bridge, so it can be driven by
[kvm-serial](https://github.com/sjmf/kvm-serial) or any other CH9329 software.

```
controlling computer ──USB──> "COM" port  (onboard USB-UART bridge → UART0, 115200 8N1)
target computer     <──USB──  "USB" port  (native USB OTG → HID keyboard + mouse)
```

Leave the board's 5V "USB-OTG"/"IN-OUT" jumper open so the two hosts' VBUS are not tied together.

## Protocol

Frames `57 AB | addr | cmd | len | data | sum` (sum = all previous bytes mod 256).

| cmd  | payload                                   | HID output                         |
|------|-------------------------------------------|------------------------------------|
| 0x01 | —                                         | replies version/USB status/LEDs    |
| 0x02 | `mod 00 k1..k6`                           | boot keyboard report               |
| 0x04 | `02 btn xL xH yL yH wheel` (x/y 0..4095)  | absolute mouse                     |
| 0x05 | `01 btn dx dy wheel`                      | relative mouse                     |

Every command is acknowledged like the real chip (`57 AB 00 cmd|80 01 status sum`).

## Build / flash / test

```sh
pio test -e native            # parser unit tests on the host
pio run -e esp32s3 -t upload  # flash through the "COM" port (edit upload_port in platformio.ini)
```

In kvm-serial: Options → Serial Port = the board's COM port, **Baud Rate = 115200**, Protocol = CH9329.
