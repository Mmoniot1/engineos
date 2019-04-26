/////////////////////////////////////////////////////////////
// NOTICE: This file was heavily modified by Monica Moniot //
// Thank you to the original author, Melissa O'Neill       //
/////////////////////////////////////////////////////////////
/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *     http://www.pcg-random.org
 */
#ifndef INCLUDE__PCG_H
#define INCLUDE__PCG_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define PCG_DECL static inline
#define PCG_DECLS

////////////////////////////////////////////////////////////////////////////
// ~~~LIBRARY CONTENTS~~~
// PCG:
//     A struct that contains the entire state of a pcg rng
//
// pcg_seeds(rng, initstate, initseq):
//     Seed the rng, either seeds or seed must be called to initialize a rng
//     Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)
// pcg_seed(rng, seed):
//     Sets both the state initializer and stream id
//     to the given seed
//
// pcg_random32(rng):
//     Generate a uniformly distributed 32-bit random integer
//
// pcg_advance(rng, delta):
//     Advances the rng stream by delta steps in lg time
//     So pcg_advance(rng, 3) behaves as though pcg_random32(rng) was called 3 times
//     Also takes negative numbers to reverse the rng stream
//
// pcg_random32_in(rng, lower, upper):
//     Generate a uniformly distributed integer, r, where lower <= r <= upper
// pcg_random_uniform(rng):
//     Generate a uniformly distributed float, r, where 0 <= r < 1
// pcg_random_uniform_in(rng):
//     Generate a uniformly distributed float, r, where 0 <= r <= 1
// pcg_random_uniform_ex(rng):
//     Generate a uniformly distributed float, r, where 0 < r < 1
//
// PCGF:
//     A uint32_t that contains the entire state of a fast pcg rng.
//     Works identically to a normal pcg rng, except it is smaller and
//     faster, but produces much lower quality numbers.
//     To get the PCGF varient of any particular function, postfix it with "f"
//     (there does not exist a pcg_seedsf or pcg_advancef function)
//

typedef struct {// Only modify the internals if you are a stats PhD.
    uint64_t state;             // RNG state.  All values are possible.
    uint64_t inc;               // Controls which RNG sequence (stream) is
                                // selected. Must *always* be odd.
} PCG;
typedef uint32_t PCGF;

// If you *must* statically initialize it, use this.
#define PCG_INITIALIZER {0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL}

PCG_DECLS void pcg_seeds(PCG* rng, uint64_t initstate, uint64_t sequence);
PCG_DECL void pcg_seed(PCG* rng, uint64_t seed);

PCG_DECLS uint32_t pcg_random32(PCG* rng);

PCG_DECL void pcg_advance(PCG* rng, uint64_t delta);

PCG_DECL uint32_t pcg_random32_in(PCG* rng, uint32_t lower, uint32_t upper);
PCG_DECL float pcg_random_uniform(PCG* rng);
PCG_DECL float pcg_random_uniform_in(PCG* rng);
PCG_DECL float pcg_random_uniform_ex(PCG* rng);


PCG_DECL void pcg_seedf(PCGF* rng, uint32_t seed);

PCG_DECL uint32_t pcg_random32f(PCGF* state);

PCG_DECL uint32_t pcg_random32_inf(PCGF* rng, uint32_t lower, uint32_t upper);
PCG_DECL float pcg_random_uniformf(PCGF* rng);
PCG_DECL float pcg_random_uniform_inf(PCGF* rng);
PCG_DECL float pcg_random_uniform_exf(PCGF* rng);

uint64_t _pcg_advance_lcg_64(uint64_t state, uint64_t delta, uint64_t cur_mult, uint64_t cur_plus);
#ifdef PCG_IMPLEMENTATION
#undef PCG_IMPLEMENTATION
uint32_t pcg_random32(PCG* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (uint32_t)((xorshifted >> rot) | (xorshifted << (((uint32_t)(-(int32_t)rot)) & 31)));
}


void pcg_seeds(PCG* rng, uint64_t initstate, uint64_t sequence) {
    rng->state = 0U;
    rng->inc = (sequence << 1u) | 1u;
    pcg_random32(rng);
    rng->state += initstate;
    pcg_random32(rng);
}

uint64_t _pcg_advance_lcg_64(uint64_t state, uint64_t delta, uint64_t cur_mult, uint64_t cur_plus) {
    uint64_t acc_mult = 1u;
    uint64_t acc_plus = 0u;
    while (delta > 0) {
        if (delta & 1) {
            acc_mult *= cur_mult;
            acc_plus = acc_plus * cur_mult + cur_plus;
        }
        cur_plus = (cur_mult + 1) * cur_plus;
        cur_mult *= cur_mult;
        delta /= 2;
    }
    return acc_mult * state + acc_plus;
}
#endif

PCG_DECL void pcg_seed(PCG* rng, uint64_t seed) {
    pcg_seeds(rng, seed, seed);
}

PCG_DECL void pcg_advance(PCG* rng, uint64_t delta) {
    rng->state = _pcg_advance_lcg_64(rng->state, delta, 6364136223846793005ULL, rng->inc);
}

PCG_DECL uint32_t pcg_random32_in(PCG* rng, uint32_t lower, uint32_t upper) {
	uint32_t bound = upper - lower + 1;
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = ((uint32_t)(-(int32_t)bound)) % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg_random32(rng);
        if (r >= threshold)
            return (r % bound) + lower;
    }
}
PCG_DECL float pcg_random_uniform(PCG* rng) {
	uint32_t r = pcg_random32(rng);
	return r*(1/(((float)~0u) + 1));
}
PCG_DECL float pcg_random_uniform_in(PCG* rng) {
	uint32_t r = pcg_random32(rng);
	return ((float)r)/((float)~0u);
}
PCG_DECL float pcg_random_uniform_ex(PCG* rng) {
	uint32_t r = pcg_random32(rng);
	return (((float)r) + 1)/(((float)~0u) + 2);
}


PCG_DECL void pcg_seedf(PCGF* rng, uint32_t seed) {
	*rng = seed;
}

PCG_DECL uint32_t pcg_random32f(PCGF* state) {
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

PCG_DECL uint32_t pcg_random32_inf(PCGF* rng, uint32_t lower, uint32_t upper) {
	return (pcg_random32f(rng)%(upper - lower + 1)) + lower;
}
PCG_DECL float pcg_random_uniformf(PCGF* rng) {
	return ((float)pcg_random32f(rng))*(1/(((float)~0u) + 1));
}
PCG_DECL float pcg_random_uniform_inf(PCGF* rng) {
	return ((float)pcg_random32f(rng))/((float)~0u);
}
PCG_DECL float pcg_random_uniform_exf(PCGF* rng) {
	return (((float)pcg_random32f(rng)) + 1)/(((float)~0u) + 2);
}

}
#endif
