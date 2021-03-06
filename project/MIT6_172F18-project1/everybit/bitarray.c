/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

// Implements the ADT specified in bitarray.h as a packed array of bits; a bit
// array containing bit_sz bits will consume roughly bit_sz/8 bytes of
// memory.

#include "./bitarray.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

// ********************************* Types **********************************

// Concrete data type representing an array of bits.
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* buf;
};

// ******************** Prototypes for static functions *********************

// Rotates a subarray left by an arbitrary number of bits.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
// bit_left_amount is the number of places to rotate the
//                    subarray left
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
// static void bitarray_rotate_left(bitarray_t* const bitarray,
//                                  const size_t bit_offset,
//                                  const size_t bit_length,
//                                  const size_t bit_left_amount);

// Rotates a subarray left by one bit.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
// static void bitarray_rotate_left_one(bitarray_t* const bitarray,
//                                      const size_t bit_offset,
//                                      const size_t bit_length);

// Portable modulo operation that supports negative dividends.
//
// Many programming languages define modulo in a manner incompatible with its
// widely-accepted mathematical definition.
// http://stackoverflow.com/questions/1907565/c-python-different-behaviour-of-the-modulo-operation
// provides details; in particular, C's modulo
// operator (which the standard calls a "remainder" operator) yields a result
// signed identically to the dividend e.g., -1 % 10 yields -1.
// This is obviously unacceptable for a function which returns size_t, so we
// define our own.
//
// n is the dividend and m is the divisor
//
// Returns a positive integer r = n (mod m), in the range
// 0 <= r < m.
static size_t modulo(const ssize_t n, const size_t m);

// Produces a mask which, when ANDed with a byte, retains only the
// bit_index th byte.
//
// Example: bitmask(5) produces the byte 0b00100000.
//
// (Note that here the index is counted from right
// to left, which is different from how we represent bitarrays in the
// tests.  This function is only used by bitarray_get and bitarray_set,
// however, so as long as you always use bitarray_get and bitarray_set
// to access bits in your bitarray, this reverse representation should
// not matter.
static char bitmask(const size_t bit_index);

static void bitarray_reverse(bitarray_t* const bitarray,
                             const size_t bit_offset, const size_t bit_length);

// ******************************* Functions
// ********************************

bitarray_t* bitarray_new(const size_t bit_sz) {
  // Allocate an underlying buffer of ceil(bit_sz/8) bytes.
  char* const buf = calloc(1, (bit_sz + 7) / 8);
  if (buf == NULL) {
    return NULL;
  }

  // Allocate space for the struct.
  bitarray_t* const bitarray = malloc(sizeof(struct bitarray));
  if (bitarray == NULL) {
    free(buf);
    return NULL;
  }

  bitarray->buf = buf;
  bitarray->bit_sz = bit_sz;
  return bitarray;
}

void bitarray_free(bitarray_t* const bitarray) {
  if (bitarray == NULL) {
    return;
  }
  free(bitarray->buf);
  bitarray->buf = NULL;
  free(bitarray);
}

size_t bitarray_get_bit_sz(const bitarray_t* const bitarray) {
  return bitarray->bit_sz;
}

bool bitarray_get(const bitarray_t* const bitarray, const size_t bit_index) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to get the nth
  // bit, we want to look at the (n mod 8)th bit of the (floor(n/8)th)
  // byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to produce either a zero byte (if the bit was 0) or a nonzero byte
  // (if it wasn't).  Finally, we convert that to a boolean.
  return (bitarray->buf[bit_index / 8] & bitmask(bit_index)) ? true : false;
}

void bitarray_set(bitarray_t* const bitarray, const size_t bit_index,
                  const bool value) {
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to set the nth
  // bit, we want to set the (n mod 8)th bit of the (floor(n/8)th) byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to clear out the bit we're about to set.  We bitwise-or the result
  // with a byte that has either a 1 or a 0 in the correct place.
  bitarray->buf[bit_index / 8] =
      (bitarray->buf[bit_index / 8] & ~bitmask(bit_index)) |
      (value ? bitmask(bit_index) : 0);
}

void bitarray_set_byte(bitarray_t* const bitarray, const size_t byte_index,
                       const unsigned char byte) {
  assert(byte_index < (bitarray->bit_sz + 7) / 8);
  bitarray->buf[byte_index] = (char)byte;
}

unsigned char bitarray_get_byte(const bitarray_t* const bitarray,
                                const size_t byte_index) {
  assert(byte_index < (bitarray->bit_sz + 7) / 8);
  return bitarray->buf[byte_index];
}

void bitarray_randfill(bitarray_t* const bitarray) {
  int32_t* ptr = (int32_t*)bitarray->buf;
  for (int64_t i = 0; i < bitarray->bit_sz / 32 + 1; i++) {
    ptr[i] = rand();
  }
}

void bitarray_rotate(bitarray_t* const bitarray, const size_t bit_offset,
                     const size_t bit_length, const ssize_t bit_right_amount) {
  assert(bit_offset + bit_length <= bitarray->bit_sz);

  if (bit_length == 0) {
    return;
  }

  const size_t bit_left_amount = modulo(-bit_right_amount, bit_length);
  if (bit_left_amount == 0) {
    return;
  }
  bitarray_reverse(bitarray, bit_offset, bit_left_amount);
  bitarray_reverse(bitarray, bit_offset + bit_left_amount,
                   bit_length - bit_left_amount);
  bitarray_reverse(bitarray, bit_offset, bit_length);
}

static inline size_t modulo(const ssize_t n, const size_t m) {
  const ssize_t signed_m = (ssize_t)m;
  assert(signed_m > 0);
  const ssize_t result = ((n % signed_m) + signed_m) % signed_m;
  assert(result >= 0);
  return (size_t)result;
  //   return n % m;
}

static char bitmask(const size_t bit_index) { return 1 << (bit_index % 8); }

// Reverses a byte using a 256 bit lookup table
char reverse_byte(unsigned char byte) {
  static const unsigned char t[256] = {
      0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
      0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
      0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
      0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
      0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
      0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
      0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
      0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
      0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
      0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
      0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
      0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
      0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
      0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
      0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
      0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
      0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
      0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
      0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
      0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
      0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
      0x3F, 0xBF, 0x7F, 0xFF};
  return (unsigned char)t[byte];
}

static inline void bitarray_reverse(bitarray_t* const bitarray,
                                    const size_t bit_offset,
                                    const size_t bit_length) {
  if (bit_length <= 1) {
    return;
  }
  size_t bit_end = bit_offset + bit_length - 1, bit_start = bit_offset;
  size_t start_index = bit_offset / 8, end_index = bit_end / 8;
  size_t fill_start_length = modulo(bit_start, 8),
         fill_end_length = modulo(8 - (bit_end + 1), 8);
  unsigned char first_byte = bitarray_get_byte(bitarray, start_index),
                end_byte = bitarray_get_byte(bitarray, end_index);

  for (size_t i = 0; i <= (end_index - start_index) >> 1; i++) {
    unsigned char tmp =
        reverse_byte(bitarray_get_byte(bitarray, start_index + i));
    bitarray_set_byte(bitarray, start_index + i,
                      reverse_byte(bitarray_get_byte(bitarray, end_index - i)));
    bitarray_set_byte(bitarray, end_index - i, tmp);
  }

  ssize_t shift = fill_end_length - fill_start_length;
  if (shift > 0) {
    for (size_t i = start_index; i < end_index; i++) {
      unsigned char x = bitarray_get_byte(bitarray, i) >> shift;
      unsigned char y = bitarray_get_byte(bitarray, i + 1) << (8 - shift);
      bitarray_set_byte(bitarray, i, x | y);
    }
    unsigned char x = bitarray_get_byte(bitarray, end_index) >> shift;
    unsigned char y = (end_byte >> (8 - shift)) << (8 - shift);
    bitarray_set_byte(bitarray, end_index, x | y);
  } else if (shift < 0) {
    shift = -shift;
    for (size_t i = end_index; i > start_index; i--) {
      unsigned char x = bitarray_get_byte(bitarray, i) << shift;
      unsigned char y = bitarray_get_byte(bitarray, i - 1) >> (8 - shift);
      bitarray_set_byte(bitarray, i, x | y);
    }
    unsigned char x = (bitarray_get_byte(bitarray, start_index) << shift);
    unsigned char y =
        ((unsigned char)(first_byte << (8 - shift))) >> (8 - shift);
    bitarray_set_byte(bitarray, start_index, x | y);
  }

  if (fill_start_length == 0 || fill_end_length == 0) {
    return;
  }

  if (fill_start_length != 0) {
    unsigned char start_x =
        ((unsigned char)(first_byte << (8 - fill_start_length)) >>
         (8 - fill_start_length));
    unsigned char start_y =
        ((unsigned char)(bitarray_get_byte(bitarray, start_index) >>
                         fill_start_length))
        << fill_start_length;
    bitarray_set_byte(bitarray, start_index, start_x | start_y);
  }
  if (fill_end_length != 0) {
    unsigned char end_x = (end_byte >> (8 - fill_end_length))
                          << (8 - fill_end_length);
    unsigned char end_y =
        (bitarray_get_byte(bitarray, end_index) << fill_end_length) >>
        fill_end_length;
    bitarray_set_byte(bitarray, end_index, end_x | end_y);
  }
}