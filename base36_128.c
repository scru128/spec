/** base36_128.c - Base36 reference implementation for 128-bit data */

#include <assert.h>
#include <stdint.h>

/**
 * Converts a digit value array in `in_base` into that in `out_base`.
 *
 * This function converts digit value arrays in any pair of bases from 2 to 256,
 * while in this file it is used only to convert between Base36 and Base256
 * (i.e., bytes). Conversion of a digit value array from/to a string is taken
 * care of by `encode()` and `decode()` functions.
 *
 * This function includes naive code (turned on by the `USE_NAIVE_CODE` flag)
 * that focuses on the illustration of the algorithm and optimized code that
 * demonstrates typical optimization approaches.
 */
static int convert_base(const uint8_t *in, int in_len, int in_base,
                        uint8_t *out, int out_len, int out_base) {
  for (int i = 0; i < out_len; i++) {
    out[i] = 0;
  }

#ifdef USE_NAIVE_CODE
  for (int i = 0; i < in_len; i++) {
    // read one digit from `in` for each outer loop
    uint_fast16_t carry = in[i];

    // fill in `out` from right to left, while carrying up prior result to left
    for (int j = out_len - 1; j >= 0; j--) {
      carry += out[j] * in_base;
      out[j] = carry % out_base;
      carry = carry / out_base;
    }
    if (carry != 0) {
      return -1; // too small out_len
    }
  }
#else
  /*
   * The refined code below applies the following techniques to the naive code:
   *
   * - use larger `carry` and read multiple `in` digits for each outer loop
   * - break inner loop when `carry` is zero and none of remaining `out` digits
   *   has been updated from initial value (zero)
   */

  // determine number of `in` digits to read for each outer loop
  int word_len = 1;
  uint64_t word_base = in_base; // set to in_base ^ word_len
  while (word_base <= UINT64_MAX / ((uint64_t)in_base * out_base)) {
    word_len++;
    word_base *= in_base;
  }

  int out_used = out_len - 1; // storage to memorize range of `out` filled

  // iterate over `in` word by word, having `i` point to head of each word
  int i = in_len % word_len;
  if (i > 0) {
    i -= word_len;
  }
  for (; i < in_len; i += word_len) {
    // read multiple `in` digits for each outer loop
    uint64_t carry = 0;
    for (int j = i < 0 ? 0 : i; j < i + word_len; j++) {
      carry = carry * in_base + in[j];
    }

    for (int j = out_len - 1; j >= 0; j--) {
      carry += out[j] * word_base;
      out[j] = carry % out_base;
      carry = carry / out_base;

      // break inner loop when `carry` and remaining `out` digits are all zero
      if (carry == 0 && j <= out_used) {
        out_used = j;
        break;
      }
    }
    if (carry != 0) {
      return -1; // too small out_len
    }
  }
#endif /* #ifdef USE_NAIVE_CODE */

  return 0; // success
}

/**
 * Encodes a 128-bit byte array in a 25-digit Base36 string.
 *
 * @param bytes 16-byte byte array
 * @param out 26-byte string (25 digits and terminating NUL)
 */
void encode(const uint8_t *bytes, char *out) {
  // Base36 digit characters
  static const char DIGITS[] = "0123456789abcdefghijklmnopqrstuvwxyz";

  // convert byte array into digit value array
  uint8_t digit_values[25];
  int err = convert_base(bytes, 16, 256, digit_values, 25, 36);
  assert(err == 0);

  // convert digit value array into string
  for (int i = 0; i < 25; i++) {
    out[i] = DIGITS[digit_values[i]];
  }
  out[25] = '\0';
}

/**
 * Decodes a 128-bit byte array from a 25-digit Base36 string.
 *
 * This function assumes target environments where `char` is compatible with
 * ASCII.
 *
 * @param text 26-byte string (25 digits and terminating NUL)
 * @param out 16-byte byte array
 * @return zero on success or non-zero on failure
 */
int decode(const char *text, uint8_t *out) {
  // O(1) map from ASCII code points to Base36 digit values
  static const uint8_t DECODE_MAP[128] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
      0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
      0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
      0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0x21, 0x22, 0x23, 0xff, 0xff, 0xff, 0xff, 0xff};

  // convert string into digit value array
  uint8_t digit_values[25];
  for (int i = 0; i < 25; i++) {
    unsigned char code = text[i];
    if (code > 127 || DECODE_MAP[code] == 0xff) {
      return -1; // invalid digit character
    }
    digit_values[i] = DECODE_MAP[code];
  }
  if (text[25] != '\0') {
    return -1; // invalid length
  }

  // convert digit value array into byte array
  int err = convert_base(digit_values, 25, 36, out, 16, 256);
  if (err != 0) {
    return -1; // out of 128-bit value range
  }

  return 0; // success
}

/** Executes the implementation against prepared test cases. */
static void test_positive_cases(void) {
  struct TestCase {
    uint8_t bytes[16];
    char text[26];
  };

  const struct TestCase test_vector[] = {
      {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
       "0000000000000000000000000"},
      {{0x01, 0x7f, 0xee, 0x7f, 0xef, 0x41, 0x7e, 0x2b, 0x34, 0x32, 0xac, 0x2e,
        0xc5, 0x53, 0x68, 0x7c},
       "0372hg16csmsm50l8dikcvukc"},
      {{0x01, 0x7f, 0xee, 0x7f, 0xef, 0x42, 0x7e, 0x2b, 0x34, 0x6c, 0x0f, 0xf4,
        0x14, 0xbb, 0xcf, 0xfd},
       "0372hg16cy3nowracls909wcd"},
      {{0x01, 0x7f, 0xef, 0x39, 0xc2, 0x64, 0x1b, 0xa5, 0x6a, 0x94, 0x83, 0x18,
        0x88, 0x41, 0xe0, 0x5a},
       "0372ijojuxuhjsfkeryi2mrtm"},
      {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff},
       "f5lxx1zz5pnorynqglhzmsp33"}};
  const int N_CASES = 5;

  for (int i = 0; i < N_CASES; i++) {
    const struct TestCase *e = &test_vector[i];

    char out_text[26];
    encode(e->bytes, out_text);
    for (int j = 0; j < 26; j++) {
      assert(e->text[j] == out_text[j]);
    }

    uint8_t out_bytes[16];
    int err = decode(e->text, out_bytes);
    assert(err == 0);
    for (int j = 0; j < 16; j++) {
      assert(e->bytes[j] == out_bytes[j]);
    }
  }
}

/** Executes the implementation against test cases that return error. */
static void test_negative_cases(void) {
  uint8_t out_bytes[16];
  int err;
  err = decode("0", out_bytes);
  assert(err != 0);
  err = decode("00000000000000000000000000", out_bytes);
  assert(err != 0);
  err = decode("f5lxx1zz5pn+rynqglhzmsp33", out_bytes);
  assert(err != 0);
  err = decode("f5lxx1zz5pnorynqglhzmsp34", out_bytes);
  assert(err != 0);
  err = decode("zzzzzzzzzzzzzzzzzzzzzzzzz", out_bytes);
  assert(err != 0);
}

int main(void) {
  test_positive_cases();
  test_negative_cases();
  return 0;
}
