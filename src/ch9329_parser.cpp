#include "ch9329_parser.h"

namespace ch9329 {

void Parser::reset() {
    state_ = WAIT_HEAD1;
    pos_ = 0;
    sum_ = 0;
}

bool Parser::feed(uint8_t b) {
    switch (state_) {
    case WAIT_HEAD1:
        if (b == HEAD1) {
            sum_ = b;
            state_ = WAIT_HEAD2;
        }
        return false;
    case WAIT_HEAD2:
        if (b == HEAD2) {
            sum_ += b;
            state_ = WAIT_ADDR;
        } else if (b != HEAD1) {
            reset();
        }  // else: 0x57 0x57 0xAB, stay waiting for HEAD2
        return false;
    case WAIT_ADDR:
        addr_ = b;
        sum_ += b;
        state_ = WAIT_CMD;
        return false;
    case WAIT_CMD:
        cmd_ = b;
        sum_ += b;
        state_ = WAIT_LEN;
        return false;
    case WAIT_LEN:
        if (b > MAX_DATA) {
            reset();
            return false;
        }
        len_ = b;
        sum_ += b;
        pos_ = 0;
        state_ = len_ ? WAIT_DATA : WAIT_SUM;
        return false;
    case WAIT_DATA:
        data_[pos_++] = b;
        sum_ += b;
        if (pos_ == len_) state_ = WAIT_SUM;
        return false;
    case WAIT_SUM: {
        bool ok = (b == sum_);
        if (!ok) checksumErrors_++;
        reset();
        return ok;
    }
    }
    reset();
    return false;
}

size_t buildFrame(uint8_t addr, uint8_t cmd, const uint8_t* data, uint8_t len, uint8_t* out) {
    size_t n = 0;
    out[n++] = HEAD1;
    out[n++] = HEAD2;
    out[n++] = addr;
    out[n++] = cmd;
    out[n++] = len;
    for (uint8_t i = 0; i < len; i++) out[n++] = data[i];
    uint8_t sum = 0;
    for (size_t i = 0; i < n; i++) sum += out[i];
    out[n++] = sum;
    return n;
}

}  // namespace ch9329
