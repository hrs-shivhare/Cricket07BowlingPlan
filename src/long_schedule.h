#ifndef CRICKET07_BOWLING_PLAN_LONG_SCHEDULE_H
#define CRICKET07_BOWLING_PLAN_LONG_SCHEDULE_H

#include "schedule.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct C07LongPlan {
    unsigned overs;
    /* End boundaries, in completed overs: powerplay, normal, random, death. */
    unsigned phase_end[4];
    unsigned total_quota[6];
    unsigned char bowler[50]; /* Zero-based B1..B6. */
    unsigned search_nodes;
} C07LongPlan;

/*
 * Supports 40, 45 and 50 overs. Fixed phases preserve the supplied 50-over
 * order, deleting selected entries for 40/45. Random-phase spells consist of
 * two successive turns from the same end. Each odd random-phase quota adds
 * exactly one separate single-turn spell; adjacent overs never share a bowler.
 *
 * Resume semantics match the short scheduler: a compatible historical prefix
 * is reconstructed from completed_counts and last_bowler. It need not have
 * the actual historical ordering. Persist the full plan when exact spell
 * continuity through saved-game reloads is required.
 *
 * At completed_overs == 0, counts may be NULL and last_bowler must be 6.
 * Otherwise counts has six entries and last_bowler is zero-based 0..5.
 * All errors leave out untouched. Search is bounded at 250,000 states;
 * NO_SCHEDULE can also indicate that budget was reached. Randomized feasible
 * plans are reproducible for a reproducible RNG, not uniformly distributed
 * over all possible full plans.
 */
c07_plan_result c07_make_long_bowling_plan(unsigned overs,
                                        c07_rng_fn rng, void *context,
                                        unsigned completed_overs,
                                        const unsigned completed_counts[6],
                                        unsigned last_bowler,
                                        C07LongPlan *out);

#ifdef __cplusplus
}
#endif
#endif
