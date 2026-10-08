#pragma once

#include <stddef.h>
#include <stdint.h>

// CH9329 frame: 0x57 0xAB | addr | cmd | len | data[len] | sum
// sum = (sum of all preceding bytes) & 0xFF
namespace ch9329 {

constexpr uint8_t HEAD1 = 0x57;
constexpr uint8_t HEAD2 = 0xAB;
constexpr size_t MAX_DATA = 64;

constexpr uint8_t CMD_GET_INFO = 0x01;
constexpr uint8_t CMD_SEND_KB_GENERAL_DATA = 0x02;
constexpr uint8_t CMD_SEND_MS_ABS_DATA = 0x04;
constexpr uint8_t CMD_SEND_MS_REL_DATA = 0x05;

constexpr uint8_t STATUS_SUCCESS = 0x00;
constexpr uint8_t STATUS_ERR_CMD = 0xE3;
constexpr uint8_t STATUS_ERR_PARAM = 0xE5;

constexpr uint16_t ABS_MAX = 4095;

class Parser {
public:
    // Feed one byte. Returns true when a complete, valid frame has been
    // received; it is then available through addr()/cmd()/data()/len()
    // until the next call to feed().
    bool feed(uint8_t b);
    void reset();

    uint8_t addr() const { return addr_; }
    uint8_t cmd() const { return cmd_; }
    uint8_t len() const { return len_; }
    const uint8_t* data() const { return data_; }

    uint32_t checksumErrors() const { return checksumErrors_; }

private:
    enum State { WAIT_HEAD1, WAIT_HEAD2, WAIT_ADDR, WAIT_CMD, WAIT_LEN, WAIT_DATA, WAIT_SUM };

    State state_ = WAIT_HEAD1;
    uint8_t addr_ = 0;
    uint8_t cmd_ = 0;
    uint8_t len_ = 0;
    uint8_t pos_ = 0;
    uint8_t sum_ = 0;
    uint8_t data_[MAX_DATA] = {};
    uint32_t checksumErrors_ = 0;
};

// Build a response frame into out (needs 6 + len bytes). Returns frame size.
size_t buildFrame(uint8_t addr, uint8_t cmd, const uint8_t* data, uint8_t len, uint8_t* out);

// Clamp an absolute coordinate from the wire into 0..ABS_MAX.
inline uint16_t clampAbs(uint16_t v) { return v > ABS_MAX ? ABS_MAX : v; }

}  // namespace ch9329
