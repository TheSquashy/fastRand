#include <sys/random.h>
#include <hwy/highway.h>

#include <stdint.h>

namespace hn = hwy::HWY_NAMESPACE;

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

//no long jumps because im lazy 
//you could implement them if you need em


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
