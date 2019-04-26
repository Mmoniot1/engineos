//By Monica Moniot
#include <math.h>
#include <stdint.h>


#define EPSILON .0000000000001
#define EPSILONF .0000000001f

#ifdef __cplusplus
#define MAMI__DELCS static inline
#else
#define MAMI__DELCS static inline
#endif

MAMI__DELCS float mam_log10f(float n) {
	return log10f(n);
}
MAMI__DELCS double mam_log10(double n) {
	return log10(n);
}

MAMI__DELCS double mam_pow10(double n) {
	return pow(10.0, n);
}
MAMI__DELCS float mam_pow10f(float n) {
	return powf(10.0, n);
}

MAMI__DELCS int32_t mam_round_to_int32(double n) {
	return (int32_t)round(n);
}
MAMI__DELCS int64_t mam_round_to_int64(double n) {
	return (int64_t)round(n);
}
MAMI__DELCS int32_t mam_floor_to_int32(double n) {
	return (int32_t)floor(n);
}
MAMI__DELCS int64_t mam_floor_to_int64(double n) {
	return (int64_t)floor(n);
}
