#include <string.h>
#include <unity.h>

#include "ch9329_parser.h"

using namespace ch9329;

static bool feedAll(Parser& p, const uint8_t* bytes, size_t n, int* frames = nullptr) {
    bool last = false;
    for (size_t i = 0; i < n; i++) {
        last = p.feed(bytes[i]);
        if (last && frames) (*frames)++;
    }
    return last;
}

// Bytes produced by kvm_serial's CH9329Comm.send_scancode(b"\x00\x00\x04\x00\x00\x00\x00\x00") ("a")
static const uint8_t KB_A[] = {0x57, 0xAB, 0x00, 0x02, 0x08, 0x00, 0x00, 0x04, 0, 0, 0, 0, 0, 0x10};

void setUp() {}
void tearDown() {}

void test_keyboard_frame() {
    Parser p;
    TEST_ASSERT_TRUE(feedAll(p, KB_A, sizeof(KB_A)));
    TEST_ASSERT_EQUAL_HEX8(0x02, p.cmd());
    TEST_ASSERT_EQUAL(8, p.len());
    TEST_ASSERT_EQUAL_HEX8(0x04, p.data()[2]);
}

void test_abs_mouse_frame() {
    // send_mouse_absolute(0, 960, 540, 1920, 1080) -> x=2048, y=2048
    uint8_t data[] = {0x02, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00};
    uint8_t frame[32];
    size_t n = buildFrame(0x00, CMD_SEND_MS_ABS_DATA, data, sizeof(data), frame);
    Parser p;
    TEST_ASSERT_TRUE(feedAll(p, frame, n));
    TEST_ASSERT_EQUAL_HEX8(CMD_SEND_MS_ABS_DATA, p.cmd());
    TEST_ASSERT_EQUAL(2048, p.data()[2] | (p.data()[3] << 8));
}

void test_bad_checksum_rejected_and_recovers() {
    uint8_t bad[sizeof(KB_A)];
    memcpy(bad, KB_A, sizeof(KB_A));
    bad[sizeof(bad) - 1] ^= 0xFF;
    Parser p;
    TEST_ASSERT_FALSE(feedAll(p, bad, sizeof(bad)));
    TEST_ASSERT_EQUAL(1, p.checksumErrors());
    TEST_ASSERT_TRUE(feedAll(p, KB_A, sizeof(KB_A)));
}

void test_resync_after_garbage() {
    // Boot ROM noise, a stray header byte, and a repeated HEAD1 before a real frame.
    const uint8_t garbage[] = {'E', 'S', 'P', '-', 'R', 'O', 'M', 0x57, 0x00, 0xAB, 0x57};
    Parser p;
    int frames = 0;
    feedAll(p, garbage, sizeof(garbage), &frames);
    feedAll(p, KB_A, sizeof(KB_A), &frames);
    TEST_ASSERT_EQUAL(1, frames);
}

void test_back_to_back_frames() {
    Parser p;
    int frames = 0;
    for (int i = 0; i < 3; i++) feedAll(p, KB_A, sizeof(KB_A), &frames);
    TEST_ASSERT_EQUAL(3, frames);
}

void test_oversized_len_rejected() {
    const uint8_t frame[] = {0x57, 0xAB, 0x00, 0x02, 0xFF};
    Parser p;
    int frames = 0;
    feedAll(p, frame, sizeof(frame), &frames);
    feedAll(p, KB_A, sizeof(KB_A), &frames);
    TEST_ASSERT_EQUAL(1, frames);
}

void test_build_ack_frame() {
    // Real CH9329 ACK for a keyboard command: 57 AB 00 82 01 00 85
    uint8_t status = STATUS_SUCCESS;
    uint8_t out[16];
    size_t n = buildFrame(0x00, 0x82, &status, 1, out);
    const uint8_t expected[] = {0x57, 0xAB, 0x00, 0x82, 0x01, 0x00, 0x85};
    TEST_ASSERT_EQUAL(sizeof(expected), n);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, n);
}

void test_clamp_abs() {
    TEST_ASSERT_EQUAL(4095, clampAbs(4096));
    TEST_ASSERT_EQUAL(100, clampAbs(100));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_keyboard_frame);
    RUN_TEST(test_abs_mouse_frame);
    RUN_TEST(test_bad_checksum_rejected_and_recovers);
    RUN_TEST(test_resync_after_garbage);
    RUN_TEST(test_back_to_back_frames);
    RUN_TEST(test_oversized_len_rejected);
    RUN_TEST(test_build_ack_frame);
    RUN_TEST(test_clamp_abs);
    return UNITY_END();
}
