#include "long_schedule.h"
#include <string.h>

#define LONG_MAX_NODES 250000U
#define LONG_MEMO_SIZE 4096U
#define LONG_MEMO_PROBES 8U

typedef struct long_spec {
    unsigned overs;
    unsigned end[4];
    unsigned quota[6];
    unsigned char powerplay[10];
    unsigned char normal[10];
    unsigned char death[10];
} long_spec;

static const long_spec specs[] = {
    {50, {10, 20, 40, 50}, {10, 10, 10, 10, 6, 4},
     {0, 1, 0, 1, 0, 1, 0, 1, 2, 3},
     {2, 3, 4, 2, 4, 2, 4, 3, 4, 3},
     {2, 0, 2, 1, 0, 1, 0, 1, 0, 1}},
    {45, {9, 18, 36, 45}, {9, 9, 9, 9, 5, 4},
     {0, 1, 0, 1, 0, 1, 0, 2, 3},
     {2, 3, 4, 2, 4, 2, 3, 4, 3},
     {2, 0, 2, 1, 0, 1, 0, 1, 0}},
    {40, {8, 16, 32, 40}, {8, 8, 8, 8, 5, 3},
     {0, 1, 0, 1, 0, 1, 2, 3},
     {2, 3, 4, 2, 4, 3, 4, 3},
     {2, 0, 2, 1, 0, 1, 0, 1}}
};

typedef struct long_state {
    unsigned char remaining[6];
    unsigned char last_end[2];
    unsigned char owed[2];
    unsigned char singles;
} long_state;

typedef struct long_search {
    c07_rng_fn rng;
    void *context;
    unsigned length;
    unsigned checkpoint;
    unsigned target[6];
    unsigned middle_quota[6];
    unsigned checkpoint_last;
    unsigned before;
    unsigned after;
    unsigned nodes;
    unsigned exhausted;
    uint64_t failed[LONG_MEMO_SIZE];
    C07LongPlan plan;
} long_search;

typedef struct long_choice {
    unsigned char bowler;
    unsigned char length;
} long_choice;

static uint32_t bounded_random(long_search *s, uint32_t bound)
{
    uint32_t v;
    const uint32_t threshold = (UINT32_C(0) - bound) % bound;
    do {
        v = s->rng(s->context);
    } while (v < threshold);
    return v % bound;
}

static uint64_t state_key(const long_state *state, unsigned position)
{
    uint64_t key = position;
    unsigned i;
    for (i = 0; i < 6; ++i)
        key = (key << 3) | state->remaining[i];
    for (i = 0; i < 2; ++i) {
        key = (key << 3) | state->last_end[i];
        key = (key << 1) | state->owed[i];
    }
    key = (key << 6) | state->singles;
    return key + 1; /* Zero is the empty hash-table sentinel. */
}

static unsigned memo_slot(uint64_t key)
{
    key ^= key >> 33;
    key *= UINT64_C(0xff51afd7ed558ccd);
    key ^= key >> 33;
    return (unsigned)key & (LONG_MEMO_SIZE - 1);
}

static int was_failed(const long_search *s, uint64_t key)
{
    unsigned i, slot = memo_slot(key);
    for (i = 0; i < LONG_MEMO_PROBES; ++i) {
        uint64_t found = s->failed[(slot + i) & (LONG_MEMO_SIZE - 1)];
        if (found == key)
            return 1;
        if (!found)
            return 0;
    }
    return 0;
}

static void memo_failure(long_search *s, uint64_t key)
{
    unsigned i, slot = memo_slot(key);
    for (i = 0; i < LONG_MEMO_PROBES; ++i) {
        uint64_t *entry = &s->failed[(slot + i) & (LONG_MEMO_SIZE - 1)];
        if (!*entry || *entry == key) {
            *entry = key;
            return;
        }
    }
    /* A full collision group only reduces pruning; it cannot reject a plan. */
}

static int search_middle(long_search *s, const long_state *state,
                         unsigned position)
{
    long_choice choices[12];
    unsigned i, count = 0, end = position & 1;
    unsigned previous = position ? state->last_end[end ^ 1] : s->before;
    uint64_t key;
    if (++s->nodes > LONG_MAX_NODES) {
        s->exhausted = 1;
        return 0;
    }
    if (position <= s->checkpoint) {
        for (i = 0; i < 6; ++i) {
            unsigned used = s->middle_quota[i] - state->remaining[i];
            if (used > s->target[i] ||
                s->target[i] - used > s->checkpoint - position)
                return 0;
        }
        if (position == s->checkpoint && position &&
            s->checkpoint_last < 6 && previous != s->checkpoint_last)
            return 0;
    }
    if (position == s->length) {
        if (previous == s->after || state->owed[0] || state->owed[1] ||
            state->singles)
            return 0;
        for (i = 0; i < 6; ++i)
            if (state->remaining[i])
                return 0;
        return 1;
    }
    key = state_key(state, position);
    if (was_failed(s, key))
        return 0;
    if (state->owed[end]) {
        i = state->last_end[end];
        if (i != previous && state->remaining[i]) {
            choices[count].bowler = (unsigned char)i;
            choices[count++].length = 0; /* Second turn of the active spell. */
        }
    } else {
        for (i = 0; i < 6; ++i) {
            if (i == previous || i == state->last_end[end])
                continue;
            if (state->remaining[i] >= 2 &&
                (s->length - position + 1) / 2 >= 2) {
                choices[count].bowler = (unsigned char)i;
                choices[count++].length = 2;
            }
            if (state->remaining[i] && (state->singles & (1U << i))) {
                choices[count].bowler = (unsigned char)i;
                choices[count++].length = 1;
            }
        }
    }
    for (i = count; i > 1; --i) {
        unsigned j = bounded_random(s, i);
        long_choice temp = choices[i - 1];
        choices[i - 1] = choices[j];
        choices[j] = temp;
    }
    for (i = 0; i < count; ++i) {
        long_state next = *state;
        unsigned bowler = choices[i].bowler;
        --next.remaining[bowler];
        next.last_end[end] = (unsigned char)bowler;
        next.owed[end] = choices[i].length == 2 ? 1 : 0;
        if (choices[i].length == 1)
            next.singles &= (unsigned char)~(1U << bowler);
        s->plan.bowler[s->plan.phase_end[1] + position] =
            (unsigned char)bowler;
        if (search_middle(s, &next, position + 1))
            return 1;
        if (s->exhausted)
            return 0;
    }
    memo_failure(s, key);
    return 0;
}

c07_plan_result c07_make_long_bowling_plan(unsigned overs,
                                        c07_rng_fn rng, void *context,
                                        unsigned completed,
                                        const unsigned counts[6],
                                        unsigned last,
                                        C07LongPlan *out)
{
    const long_spec *spec = 0;
    long_search s;
    long_state state;
    unsigned fixed_total[6] = {0}, fixed_prefix[6] = {0};
    unsigned i, total = 0, target_total = 0;
    if (!out || !rng)
        return C07_PLAN_INVALID_ARGUMENT;
    for (i = 0; i < sizeof(specs) / sizeof(specs[0]); ++i)
        if (specs[i].overs == overs)
            spec = &specs[i];
    if (!spec)
        return C07_PLAN_UNSUPPORTED;
    if (completed > overs || (completed && (!counts || last >= 6)) ||
        (!completed && last != 6))
        return C07_PLAN_INVALID_ARGUMENT;
    memset(&s, 0, sizeof(s));
    memset(&state, 0, sizeof(state));
    s.rng = rng;
    s.context = context;
    s.plan.overs = overs;
    memcpy(s.plan.phase_end, spec->end, sizeof(spec->end));
    memcpy(s.plan.total_quota, spec->quota, sizeof(spec->quota));
    memcpy(s.plan.bowler, spec->powerplay, spec->end[0]);
    memcpy(s.plan.bowler + spec->end[0], spec->normal,
           spec->end[1] - spec->end[0]);
    memcpy(s.plan.bowler + spec->end[2], spec->death,
           spec->end[3] - spec->end[2]);
    for (i = 0; i < overs; ++i) {
        if (i >= spec->end[1] && i < spec->end[2])
            continue;
        ++fixed_total[s.plan.bowler[i]];
        if (i < completed)
            ++fixed_prefix[s.plan.bowler[i]];
    }
    s.length = spec->end[2] - spec->end[1];
    s.checkpoint = completed > spec->end[1] ? completed - spec->end[1] : 0;
    if (s.checkpoint > s.length)
        s.checkpoint = s.length;
    s.checkpoint_last = completed > spec->end[1] &&
                        completed <= spec->end[2] ? last : 6;
    s.before = s.plan.bowler[spec->end[1] - 1];
    s.after = s.plan.bowler[spec->end[2]];
    state.last_end[0] = 6;
    state.last_end[1] = 6;
    if (completed && (completed <= spec->end[1] || completed > spec->end[2]) &&
        s.plan.bowler[completed - 1] != last)
        return C07_PLAN_NO_SCHEDULE;
    for (i = 0; i < 6; ++i) {
        unsigned observed = counts ? counts[i] : 0;
        if (observed > spec->quota[i] || observed < fixed_prefix[i])
            return C07_PLAN_NO_SCHEDULE;
        total += observed;
        s.target[i] = observed - fixed_prefix[i];
        target_total += s.target[i];
        s.middle_quota[i] = spec->quota[i] - fixed_total[i];
        state.remaining[i] = (unsigned char)s.middle_quota[i];
        if (s.target[i] > s.middle_quota[i])
            return C07_PLAN_NO_SCHEDULE;
        if (s.middle_quota[i] & 1)
            state.singles |= (unsigned char)(1U << i);
    }
    if (total != completed || target_total != s.checkpoint ||
        (completed && !counts[last]))
        return C07_PLAN_NO_SCHEDULE;
    if (!search_middle(&s, &state, 0))
        return C07_PLAN_NO_SCHEDULE;
    s.plan.search_nodes = s.nodes;
    *out = s.plan;
    return C07_PLAN_OK;
}
