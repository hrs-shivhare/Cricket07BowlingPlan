#include "../src/long_schedule.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned maximum_nodes;
static unsigned resume_cases;

static uint32_t random32(void *context)
{
    uint32_t *state = (uint32_t *)context;
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void validate(const C07LongPlan *p)
{
    static const unsigned q50[6] = {10, 10, 10, 10, 6, 4};
    static const unsigned q45[6] = {9, 9, 9, 9, 5, 4};
    static const unsigned q40[6] = {8, 8, 8, 8, 5, 3};
    static const unsigned m50[6] = {2, 2, 4, 6, 2, 4};
    static const unsigned m45[6] = {1, 3, 3, 5, 2, 4};
    static const unsigned m40[6] = {2, 2, 3, 4, 2, 3};
    static const unsigned char pp50[] = {0, 1, 0, 1, 0, 1, 0, 1, 2, 3};
    static const unsigned char n50[] = {2, 3, 4, 2, 4, 2, 4, 3, 4, 3};
    static const unsigned char d50[] = {2, 0, 2, 1, 0, 1, 0, 1, 0, 1};
    static const unsigned char pp45[] = {0, 1, 0, 1, 0, 1, 0, 2, 3};
    static const unsigned char n45[] = {2, 3, 4, 2, 4, 2, 3, 4, 3};
    static const unsigned char d45[] = {2, 0, 2, 1, 0, 1, 0, 1, 0};
    static const unsigned char pp40[] = {0, 1, 0, 1, 0, 1, 2, 3};
    static const unsigned char n40[] = {2, 3, 4, 2, 4, 3, 4, 3};
    static const unsigned char d40[] = {2, 0, 2, 1, 0, 1, 0, 1};
    const unsigned *quota, *middle_quota;
    const unsigned char *pp, *normal, *death;
    unsigned i, end, counts[6] = {0}, middle[6] = {0}, single[6] = {0};
    unsigned boundary = p->overs / 5;
    if (p->overs == 50) {
        quota = q50; middle_quota = m50;
        pp = pp50; normal = n50; death = d50;
    } else if (p->overs == 45) {
        quota = q45; middle_quota = m45;
        pp = pp45; normal = n45; death = d45;
    } else {
        assert(p->overs == 40);
        quota = q40; middle_quota = m40;
        pp = pp40; normal = n40; death = d40;
    }
    assert(p->phase_end[0] == boundary);
    assert(p->phase_end[1] == 2 * boundary);
    assert(p->phase_end[2] == 4 * boundary);
    assert(p->phase_end[3] == p->overs);
    assert(memcmp(p->bowler, pp, boundary) == 0);
    assert(memcmp(p->bowler + boundary, normal, boundary) == 0);
    assert(memcmp(p->bowler + 4 * boundary, death, boundary) == 0);
    for (i = 0; i < p->overs; ++i) {
        assert(p->bowler[i] < 6);
        ++counts[p->bowler[i]];
        if (i)
            assert(p->bowler[i] != p->bowler[i - 1]);
        if (i >= p->phase_end[1] && i < p->phase_end[2])
            ++middle[p->bowler[i]];
    }
    assert(memcmp(counts, quota, sizeof(counts)) == 0);
    assert(memcmp(p->total_quota, quota, sizeof(counts)) == 0);
    assert(memcmp(middle, middle_quota, sizeof(middle)) == 0);
    /* Validate actual same-end runs, not four-over block appearances. */
    for (end = 0; end < 2; ++end) {
        unsigned previous = 6, run = 0;
        for (i = p->phase_end[1] + end; i < p->phase_end[2]; i += 2) {
            unsigned bowler = p->bowler[i];
            if (bowler == previous) {
                ++run;
            } else {
                if (run) {
                    assert(run == 1 || run == 2);
                    if (run == 1)
                        ++single[previous];
                }
                run = 1;
                previous = bowler;
            }
        }
        assert(run == 1 || run == 2);
        if (run == 1)
            ++single[previous];
    }
    for (i = 0; i < 6; ++i)
        assert(single[i] == (middle[i] & 1));
    assert(p->search_nodes > 0 && p->search_nodes <= 250000);
    if (p->search_nodes > maximum_nodes)
        maximum_nodes = p->search_nodes;
}

static void initial_case(unsigned overs, uint32_t seed)
{
    C07LongPlan a, b;
    uint32_t first = seed, again = seed;
    assert(c07_make_long_bowling_plan(overs, random32, &first, 0, 0, 6,
                                     &a) == C07_PLAN_OK);
    assert(c07_make_long_bowling_plan(overs, random32, &again, 0, 0, 6,
                                     &b) == C07_PLAN_OK);
    assert(memcmp(&a, &b, sizeof(a)) == 0);
    validate(&a);
}

static void resume_case(unsigned overs, uint32_t seed)
{
    C07LongPlan original, resumed;
    uint32_t state = seed;
    unsigned counts[6] = {0}, checkpoint, i;
    assert(c07_make_long_bowling_plan(overs, random32, &state, 0, 0, 6,
                                     &original) == C07_PLAN_OK);
    for (checkpoint = 0; checkpoint <= overs; ++checkpoint) {
        unsigned observed[6] = {0};
        unsigned last = checkpoint ? original.bowler[checkpoint - 1] : 6;
        assert(c07_make_long_bowling_plan(overs, random32, &state, checkpoint,
                                         counts, last, &resumed) == C07_PLAN_OK);
        validate(&resumed);
        for (i = 0; i < checkpoint; ++i)
            ++observed[resumed.bowler[i]];
        assert(memcmp(counts, observed, sizeof(counts)) == 0);
        if (checkpoint)
            assert(resumed.bowler[checkpoint - 1] == last);
        if (checkpoint < overs)
            ++counts[original.bowler[checkpoint]];
        ++resume_cases;
    }
}

static void invalid_cases(void)
{
    C07LongPlan p, before;
    uint32_t state = 42;
    unsigned counts[6] = {1, 0, 0, 0, 0, 0};
    memset(&p, 0xa5, sizeof(p));
    before = p;
    assert(c07_make_long_bowling_plan(20, random32, &state, 0, 0, 6,
                                     &p) == C07_PLAN_UNSUPPORTED);
    assert(c07_make_long_bowling_plan(50, 0, &state, 0, 0, 6,
                                     &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_long_bowling_plan(50, random32, &state, 51, counts, 0,
                                     &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_long_bowling_plan(50, random32, &state, 1, 0, 0,
                                     &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_long_bowling_plan(50, random32, &state, 0, 0, 0,
                                     &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_long_bowling_plan(50, random32, &state, 1, counts, 1,
                                     &p) == C07_PLAN_NO_SCHEDULE);
    counts[0] = 0;
    counts[4] = 1;
    assert(c07_make_long_bowling_plan(50, random32, &state, 1, counts, 4,
                                     &p) == C07_PLAN_NO_SCHEDULE);
    counts[4] = 0;
    counts[0] = 11;
    assert(c07_make_long_bowling_plan(50, random32, &state, 11, counts, 0,
                                     &p) == C07_PLAN_NO_SCHEDULE);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
}

int main(void)
{
    unsigned seed;
    for (seed = 1; seed <= 500; ++seed) {
        initial_case(50, seed);
        initial_case(45, seed);
        initial_case(40, seed);
    }
    for (seed = 1; seed <= 2; ++seed) {
        resume_case(50, seed);
        resume_case(45, seed);
        resume_case(40, seed);
    }
    invalid_cases();
    printf("PASS: 1,500 seeded long plans and reproducibility, 40/45/50 quotas, fixed phases.\n");
    printf("PASS: same-end two-turn spells, exact odd-quota single spells, adjacency.\n");
    printf("PASS: %u resumes, impossible histories and output preservation; max search %u states.\n",
           resume_cases, maximum_nodes);
    return 0;
}
