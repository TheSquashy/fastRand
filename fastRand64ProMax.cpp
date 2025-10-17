#include <sys/random.h>
#include <hwy/highway.h>

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdint.h>

/* This is xoshiro256+ 1.0, our best and fastest generator for floating-point
   numbers. We suggest to use its upper bits for floating-point
   generation, as it is slightly faster than xoshiro256++/xoshiro256**. It
   passes all tests we are aware of except for the lowest three bits,
   which might fail linearity tests (and just those), so if low linear
   complexity is not considered an issue (as it is usually the case) it
   can be used to generate 64-bit outputs, too.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */


namespace hn = hwy::HWY_NAMESPACE;

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

const hn::ScalableTag<uint8_t> d8;
const size_t lanes8 = hn::Lanes(d8);
const hn::ScalableTag<uint32_t> d32;
const size_t lanes32 = hn::Lanes(d32);
const hn::ScalableTag<uint64_t> d;
const size_t lanes = hn::Lanes(d);
const size_t fullWidth = 2*lanes;

static auto s0 = hn::Zero(d);
static auto s1 = hn::Zero(d);
static auto s2 = hn::Zero(d);
static auto s3 = hn::Zero(d);

static uint64_t s[4];

HWY_ATTR auto vNext(void) {
    const auto result = hn::BitCast(d32, s0 + s3);

    const auto t = hn::ShiftLeft<17>(s1);

    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;

    s2 ^= t;

    s3 = hn::RotateLeft<45>(s3);

    return result;
}

uint64_t next(void) {
	const uint64_t result = s[0] + s[3];

	const uint64_t t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}


/* This is the jump function for the generator. It is equivalent
   to 2^128 calls to next(); it can be used to generate 2^128
   non-overlapping subsequences for parallel computations. */

void jump(void) {
	static const uint64_t JUMP[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for(int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for(int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
				s2 ^= s[2];
				s3 ^= s[3];
			}
			next();	
		}
		
	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;
}


/* This is the long-jump function for the generator. It is equivalent to
   2^192 calls to next(); it can be used to generate 2^64 starting points,
   from each of which jump() will generate 2^64 non-overlapping
   subsequences for parallel distributed computations. */

void long_jump(void) {
	static const uint64_t LONG_JUMP[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for(int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
		for(int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
				s2 ^= s[2];
				s3 ^= s[3];
			}
			next();	
		}
		
	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;
}


uint32_t rand_m(uint32_t max) {
    static const size_t iMax = fullWidth -1;
    static size_t i = iMax;
    static uint32_t random[fullWidth];
    uint32_t value = 0;
    switch (i) {
        case iMax: value = random[i]; i = 0; hn::Store(vNext(), d32, random); break;
        default: value = random[i]; i++; break;
    }
    return (((uint64_t)value * max) >> 32) +1;

}

/* 
uint32_t rand_m(uint32_t max) {
	static size_t count = 0;
	static u_int64_t random = 0;
	uint32_t tmp = 0;
	switch (count & 1) {
		case 1: tmp = random >> 32; break;
		case 0: tmp = random; random = next(); break;
	}
	return (((uint64_t)tmp * max) >> 32) +1;
}
 */
void seed() {
    uint8_t b[4][sizeof(u_int64_t)*lanes];
    for (int i = 0; i<4;i++){
        getrandom(b[i],(sizeof(u_int64_t)*lanes),0);
    }
    s0 = hn::BitCast(d, hn::Load(d8,b[0]));
    s1 = hn::BitCast(d, hn::Load(d8,b[1]));
    s2 = hn::BitCast(d, hn::Load(d8,b[2]));
    s3 = hn::BitCast(d, hn::Load(d8,b[3]));

	rand_m(32767);
}
