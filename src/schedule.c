#include "schedule.h"
#include "long_schedule.h"
#include <string.h>

/* Middle candidates are B3, B4, B5, B6, represented here as 0..3. */
typedef struct middle_state {
    unsigned length;
    unsigned char minimum[4];
    unsigned char maximum[4];
    unsigned char count[4];
    uint32_t memo[4][4][4][3][5];
} middle_state;

static int init_middle(middle_state *s, unsigned overs, unsigned third,
                       unsigned *previous)
{
    unsigned i;
    memset(s, 0, sizeof(*s));
    memset(s->memo, 0xff, sizeof(s->memo));
    if (overs == 20) {
        s->length = 10;
        for (i = 0; i < 3; ++i) {
            s->minimum[i] = 2;
            s->maximum[i] = 3;
        }
        s->minimum[3] = 1;
        s->maximum[3] = 2;
        *previous = 1; /* Over six is B4. */
        return 1;
    }
    if (overs == 10 && (third == 2 || third == 3)) {
        s->length = 5;
        for (i = 0; i < 3; ++i) {
            s->minimum[i] = 1;
            s->maximum[i] = 2;
        }
        s->maximum[third - 2] = 1; /* Already has one powerplay over. */
        s->minimum[3] = 1;
        s->maximum[3] = 1;
        *previous = third - 2;
        return 1;
    }
    return 0;
}

static uint32_t suffix_count(middle_state *s, unsigned previous)
{
    uint32_t *cached;
    uint32_t total = 0;
    unsigned used = 0, missing = 0, capacity = 0, i;
    cached = &s->memo[s->count[0]][s->count[1]][s->count[2]]
                     [s->count[3]][previous];
    if (*cached != UINT32_MAX)
        return *cached;
    for (i = 0; i < 4; ++i) {
        used += s->count[i];
        if (s->count[i] < s->minimum[i])
            missing += s->minimum[i] - s->count[i];
        capacity += s->maximum[i] - s->count[i];
    }
    if (used == s->length) {
        *cached = missing == 0 ? 1U : 0U;
        return *cached;
    }
    if (missing > s->length - used || capacity < s->length - used) {
        *cached = 0;
        return 0;
    }
    for (i = 0; i < 4; ++i) {
        if (i == previous || s->count[i] == s->maximum[i])
            continue;
        ++s->count[i];
        total += suffix_count(s, i);
        --s->count[i];
    }
    *cached = total;
    return total;
}

static uint32_t uniform_below(c07_rng_fn rng, void *context, uint32_t bound)
{
    uint32_t value;
    /* Discard the incomplete modulo bucket rather than biasing the draw. */
    const uint32_t threshold = (UINT32_C(0) - bound) % bound;
    do {
        value = rng(context);
    } while (value < threshold);
    return value % bound;
}

uint32_t c07_count_middle_sequences(unsigned overs, unsigned third)
{
    middle_state s;
    unsigned previous;
    if (!init_middle(&s, overs, third, &previous))
        return 0;
    return suffix_count(&s, previous);
}

static void init_plan(c07_bowling_plan *plan, unsigned overs, unsigned third)
{
    static const unsigned char five[] = {0, 2, 4, 3, 1};
    static const unsigned char twenty_open[] = {0, 1, 0, 1, 2, 3};
    static const unsigned char twenty_death[] = {1, 0, 1, 0};
    memset(plan, 0, sizeof(*plan));
    plan->overs = overs;
    if (overs == 5) {
        plan->powerplay_overs = 1;
        memcpy(plan->bowler, five, sizeof(five));
    } else if (overs == 20) {
        plan->powerplay_overs = 6;
        memcpy(plan->bowler, twenty_open, sizeof(twenty_open));
        memcpy(plan->bowler + 16, twenty_death, sizeof(twenty_death));
    } else {
        plan->powerplay_overs = 3;
        plan->bowler[0] = 0;
        plan->bowler[1] = 1;
        plan->bowler[2] = (unsigned char)third;
        plan->bowler[8] = 1;
        plan->bowler[9] = 0;
    }
}

static c07_plan_result make_initial(unsigned overs, unsigned third,
                                    c07_rng_fn rng, void *context,
                                    c07_bowling_plan *out)
{
    middle_state s;
    c07_bowling_plan plan;
    unsigned previous, offset, over, i;
    uint32_t total, draw, weight;
    init_plan(&plan, overs, third);
    if (overs == 5) {
        *out = plan;
        return C07_PLAN_OK;
    }
    if (!init_middle(&s, overs, third, &previous))
        return C07_PLAN_NO_SCHEDULE;
    offset = overs == 20 ? 6 : 3;
    for (over = 0; over < s.length; ++over) {
        total = suffix_count(&s, previous);
        if (!total)
            return C07_PLAN_NO_SCHEDULE;
        draw = uniform_below(rng, context, total);
        for (i = 0; i < 4; ++i) {
            if (i == previous || s.count[i] == s.maximum[i])
                continue;
            ++s.count[i];
            weight = suffix_count(&s, i);
            if (draw < weight) {
                previous = i;
                plan.bowler[offset + over] = (unsigned char)(i + 2);
                break; /* Keep the selected count increment. */
            }
            draw -= weight;
            --s.count[i];
        }
        if (i == 4)
            return C07_PLAN_NO_SCHEDULE;
    }
    *out = plan;
    return C07_PLAN_OK;
}

typedef struct resume_state {
    unsigned completed;
    const unsigned *counts;
    unsigned last;
    unsigned offset;
    c07_rng_fn rng;
    void *context;
    uint32_t matches;
    c07_bowling_plan chosen;
} resume_state;

static int prefix_matches(const c07_bowling_plan *plan, unsigned completed,
                          const unsigned counts[6], unsigned last)
{
    unsigned found[6] = {0}, i;
    if (completed && plan->bowler[completed - 1] != last)
        return 0;
    for (i = 0; i < completed; ++i)
        ++found[plan->bowler[i]];
    for (i = 0; i < 6; ++i)
        if (found[i] != counts[i])
            return 0;
    return 1;
}

static void enumerate_resume(middle_state *s, resume_state *r,
                             c07_bowling_plan *plan, unsigned depth,
                             unsigned previous)
{
    unsigned i;
    if (depth == s->length) {
        if (prefix_matches(plan, r->completed, r->counts, r->last)) {
            ++r->matches;
            /* Reservoir sampling is uniform over complete matching plans. */
            if (r->matches == 1 ||
                uniform_below(r->rng, r->context, r->matches) == 0)
                r->chosen = *plan;
        }
        return;
    }
    for (i = 0; i < 4; ++i) {
        if (i == previous || s->count[i] == s->maximum[i])
            continue;
        ++s->count[i];
        if (suffix_count(s, i)) {
            plan->bowler[r->offset + depth] = (unsigned char)(i + 2);
            enumerate_resume(s, r, plan, depth + 1, i);
        }
        --s->count[i];
    }
}

c07_plan_result c07_make_bowling_plan_at(unsigned overs, unsigned third,
                                       c07_rng_fn rng, void *context,
                                       unsigned completed,
                                       const unsigned counts[6],
                                       unsigned last,
                                       c07_bowling_plan *out)
{
    middle_state s;
    resume_state r;
    c07_bowling_plan plan;
    unsigned i, previous, sum = 0;
    if (!out)
        return C07_PLAN_INVALID_ARGUMENT;
    if (overs == 40 || overs == 45 || overs == 50) {
        C07LongPlan long_plan;
        c07_bowling_plan result;
        c07_plan_result status = c07_make_long_bowling_plan(overs, rng, context,
                                                          completed, counts, last, &long_plan);
        if (status != C07_PLAN_OK)
            return status;
        memset(&result, 0, sizeof(result));
        result.overs = overs;
        result.powerplay_overs = overs / 5;
        memcpy(result.bowler, long_plan.bowler, sizeof(result.bowler));
        *out = result;
        return C07_PLAN_OK;
    }
    if (overs != 5 && overs != 10 && overs != 20)
        return C07_PLAN_UNSUPPORTED;
    if ((overs != 5 && !rng) ||
        (overs == 10 && third != 2 && third != 3) ||
        completed > overs || (completed && (!counts || last >= 6)) ||
        (!completed && last != 6))
        return C07_PLAN_INVALID_ARGUMENT;
    if (counts) {
        for (i = 0; i < 6; ++i) {
            if (counts[i] > overs / 5)
                return C07_PLAN_NO_SCHEDULE;
            sum += counts[i];
        }
        if (sum != completed || (completed && counts[last] == 0))
            return C07_PLAN_NO_SCHEDULE;
    }
    if (!completed)
        return make_initial(overs, third, rng, context, out);
    init_plan(&plan, overs, third);
    if (overs == 5) {
        if (!prefix_matches(&plan, completed, counts, last))
            return C07_PLAN_NO_SCHEDULE;
        *out = plan;
        return C07_PLAN_OK;
    }
    if (!init_middle(&s, overs, third, &previous))
        return C07_PLAN_NO_SCHEDULE;
    memset(&r, 0, sizeof(r));
    r.completed = completed;
    r.counts = counts;
    r.last = last;
    r.offset = overs == 20 ? 6 : 3;
    r.rng = rng;
    r.context = context;
    enumerate_resume(&s, &r, &plan, 0, previous);
    if (!r.matches)
        return C07_PLAN_NO_SCHEDULE;
    *out = r.chosen;
    return C07_PLAN_OK;
}

c07_plan_result c07_make_bowling_plan(unsigned overs, unsigned third,
                                    c07_rng_fn rng, void *context,
                                    c07_bowling_plan *out)
{
    return c07_make_bowling_plan_at(overs, third, rng, context, 0, 0, 6, out);
}
