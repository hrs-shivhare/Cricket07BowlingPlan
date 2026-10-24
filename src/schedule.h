#ifndef CRICKET07_BOWLING_PLAN_SCHEDULE_H
#define CRICKET07_BOWLING_PLAN_SCHEDULE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Every callback result must be a uniform 32-bit value for uniform plans. */
typedef uint32_t (*c07_rng_fn)(void *context);

typedef struct c07_bowling_plan {
    unsigned overs;
    unsigned powerplay_overs;
    /* Zero-based bowling-order positions: 0 means B1, 5 means B6. */
    unsigned char bowler[50];
} c07_bowling_plan;

typedef enum c07_plan_result {
    C07_PLAN_OK = 0,
    C07_PLAN_UNSUPPORTED,
    C07_PLAN_INVALID_ARGUMENT,
    C07_PLAN_NO_SCHEDULE
} c07_plan_result;

/*
 * Supported lengths: 5, 10, 20, 40, 45 and 50. For ten overs, t10_third_bowler is
 * zero-based 2 (B3) or 3 (B4). It is ignored for the other formats.
 * rng is required for 10/20 overs; five overs is entirely fixed.
 * On error, out is untouched. All feasible complete middles have equal
 * probability when the callback supplies independent uniform uint32_t values.
 */
c07_plan_result c07_make_bowling_plan(unsigned overs,
                                    unsigned t10_third_bowler,
                                    c07_rng_fn rng, void *context,
                                    c07_bowling_plan *out);

/*
 * Resume from completed over totals rather than changing already-bowled overs.
 * counts contains six zero-based bowling-order counts; last_bowler is 0..5,
 * or 6 when completed_overs is zero. At zero, counts may be NULL.
 * The returned full plan has the same prefix counts and final prefix bowler;
 * its historic ordering need not equal the actual ordering. Only its remaining
 * overs should be used. Samples uniformly among all full plans matching those
 * facts. Impossible histories return C07_PLAN_NO_SCHEDULE, leaving out intact.
 */
c07_plan_result c07_make_bowling_plan_at(unsigned overs,
                                       unsigned t10_third_bowler,
                                       c07_rng_fn rng, void *context,
                                       unsigned completed_overs,
                                       const unsigned completed_counts[6],
                                       unsigned last_bowler,
                                       c07_bowling_plan *out);

/* Returns zero for unsupported lengths/invalid ten-over variants. */
uint32_t c07_count_middle_sequences(unsigned overs,
                                    unsigned t10_third_bowler);

#ifdef __cplusplus
}
#endif
#endif
