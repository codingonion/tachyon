#include "tachyon/c/crypto/random/rng.h"

#include "absl/types/span.h"

#include "tachyon/base/buffer/buffer.h"
#include "tachyon/base/logging.h"
#include "tachyon/crypto/random/xor_shift/xor_shift_rng.h"

using namespace tachyon;

tachyon_rng* tachyon_rng_create_from_seed(uint8_t type, const uint8_t* seed,
                                          size_t seed_len) {
  tachyon_rng* rng = new tachyon_rng;
  rng->type = type;
  if (type == TACHYON_RNG_XOR_SHIFT) {
    CHECK_EQ(seed_len, crypto::XORShiftRNG::kSeedSize);
    crypto::XORShiftRNG* xor_shift = new crypto::XORShiftRNG;
    *xor_shift = crypto::XORShiftRNG::FromSeed(
        absl::Span<const uint8_t>(seed, seed_len));
    rng->extra = xor_shift;
  }
  return rng;
}

tachyon_rng* tachyon_rng_create_from_state(uint8_t type, const uint8_t* state,
                                           size_t state_len) {
  tachyon_rng* rng = new tachyon_rng;
  rng->type = type;
  if (type == TACHYON_RNG_XOR_SHIFT) {
    CHECK_EQ(state_len, crypto::XORShiftRNG::kStateSize);
    base::Buffer buffer(const_cast<uint8_t*>(state), state_len);
    uint32_t x, y, z, w;
    CHECK(buffer.Read32LE(&x));
    CHECK(buffer.Read32LE(&y));
    CHECK(buffer.Read32LE(&z));
    CHECK(buffer.Read32LE(&w));
    crypto::XORShiftRNG* xor_shift = new crypto::XORShiftRNG;
    *xor_shift = crypto::XORShiftRNG::FromState(x, y, z, w);
    rng->extra = xor_shift;
  }
  return rng;
}

void tachyon_rng_destroy(tachyon_rng* rng) {
  if (rng->type == TACHYON_RNG_XOR_SHIFT) {
    delete reinterpret_cast<crypto::XORShiftRNG*>(rng->extra);
  }
  delete rng;
}

uint32_t tachyon_rng_get_next_u32(tachyon_rng* rng) {
  if (rng->type == TACHYON_RNG_XOR_SHIFT) {
    return reinterpret_cast<crypto::XORShiftRNG*>(rng->extra)->NextUint32();
  }
  NOTREACHED();
  return 0;
}

uint64_t tachyon_rng_get_next_u64(tachyon_rng* rng) {
  if (rng->type == TACHYON_RNG_XOR_SHIFT) {
    return reinterpret_cast<crypto::XORShiftRNG*>(rng->extra)->NextUint64();
  }
  NOTREACHED();
  return 0;
}

void tachyon_rng_get_state(const tachyon_rng* rng, uint8_t* state,
                           size_t* state_len) {
  if (rng->type == TACHYON_RNG_XOR_SHIFT) {
    *state_len = crypto::XORShiftRNG::kStateSize;
    if (state == nullptr) return;
    crypto::XORShiftRNG* xor_shift =
        reinterpret_cast<crypto::XORShiftRNG*>(rng->extra);
    base::Buffer buffer(state, crypto::XORShiftRNG::kStateSize);
    CHECK(buffer.Write32LE(xor_shift->x()));
    CHECK(buffer.Write32LE(xor_shift->y()));
    CHECK(buffer.Write32LE(xor_shift->z()));
    CHECK(buffer.Write32LE(xor_shift->w()));
    return;
  }
  NOTREACHED();
}
