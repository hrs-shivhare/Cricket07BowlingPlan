#include "../src/schedule.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t random32(void *context)
{
    uint32_t *s = (uint32_t *)context;
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void check_plan(unsigned overs, unsigned third, uint32_t seed)
{
    c07_bowling_plan p, duplicate;
    uint32_t state = seed, same_state = seed;
    unsigned i, count[6] = {0}, middle[6] = {0};
    unsigned start = overs == 20 ? 6 : 3;
    unsigned end = overs == 20 ? 16 : 8;
    assert(c07_make_bowling_plan(overs, third, random32, &state, &p) == C07_PLAN_OK);
    assert(c07_make_bowling_plan(overs, third, random32, &same_state,
                               &duplicate) == C07_PLAN_OK);
    assert(memcmp(&p, &duplicate, sizeof(p)) == 0);
    assert(p.overs == overs);
    assert(p.powerplay_overs == (overs == 20 ? 6U : 3U));
    for (i = 0; i < overs; ++i) {
        assert(p.bowler[i] < 6);
        ++count[p.bowler[i]];
        if (i)
            assert(p.bowler[i] != p.bowler[i - 1]);
        if (i >= start && i < end) {
            assert(p.bowler[i] >= 2);
            ++middle[p.bowler[i]];
        }
    }
    for (i = 0; i < 6; ++i)
        assert(count[i] <= overs / 5);
    assert(count[0] == overs / 5 && count[1] == overs / 5);
    if (overs == 20) {
        static const unsigned char opening[] = {0, 1, 0, 1, 2, 3};
        static const unsigned char death[] = {1, 0, 1, 0};
        assert(memcmp(p.bowler, opening, sizeof(opening)) == 0);
        assert(memcmp(p.bowler + 16, death, sizeof(death)) == 0);
        for (i = 2; i < 5; ++i)
            assert(middle[i] >= 2 && middle[i] <= 3);
        assert(middle[5] >= 1 && middle[5] <= 2);
    } else {
        assert(p.bowler[0] == 0 && p.bowler[1] == 1);
        assert(p.bowler[2] == third);
        assert(p.bowler[8] == 1 && p.bowler[9] == 0);
        for (i = 2; i < 5; ++i)
            assert(middle[i] >= 1 && middle[i] <= 2);
        assert(middle[third] == 1);
        assert(middle[5] == 1);
    }
}

/* Independent complete enumeration: deliberately does not use the DP. */
static uint32_t brute_count(unsigned overs, unsigned third, unsigned depth,
                            unsigned previous, unsigned *counts)
{
    unsigned i, length = overs == 20 ? 10 : 5;
    uint32_t total = 0;
    if (depth == length) {
        for (i = 0; i < 3; ++i) {
            unsigned minimum = overs == 20 ? 2 : 1;
            unsigned maximum = overs == 20 ? 3 : (i + 2 == third ? 1 : 2);
            if (counts[i] < minimum || counts[i] > maximum)
                return 0;
        }
        return counts[3] >= 1 && counts[3] <= (overs == 20 ? 2U : 1U);
    }
    for (i = 0; i < 4; ++i) {
        if (i == previous)
            continue;
        ++counts[i];
        total += brute_count(overs, third, depth + 1, i, counts);
        --counts[i];
    }
    return total;
}

static void check_resume(unsigned overs, unsigned third, uint32_t seed)
{
    c07_bowling_plan original, resumed;
    unsigned checkpoint, i, counts[6] = {0};
    unsigned middle_start = overs == 20 ? 6 : 3;
    unsigned middle_end = overs == 20 ? 16 : 8;
    uint32_t state = seed;
    assert(c07_make_bowling_plan(overs, third, random32, &state,
                               &original) == C07_PLAN_OK);
    for (checkpoint = 0; checkpoint <= overs; ++checkpoint) {
        unsigned observed[6] = {0}, total[6] = {0}, middle[6] = {0};
        unsigned last = checkpoint ? original.bowler[checkpoint - 1] : 6;
        assert(c07_make_bowling_plan_at(overs, third, random32, &state,
                                      checkpoint, counts, last,
                                      &resumed) == C07_PLAN_OK);
        assert(resumed.overs == overs);
        assert(resumed.powerplay_overs == original.powerplay_overs);
        for (i = 0; i < overs; ++i) {
            assert(resumed.bowler[i] < 6);
            ++total[resumed.bowler[i]];
            if (i < checkpoint)
                ++observed[resumed.bowler[i]];
            if (i)
                assert(resumed.bowler[i] != resumed.bowler[i - 1]);
            if (overs == 5 || i < middle_start || i >= middle_end)
                assert(resumed.bowler[i] == original.bowler[i]);
            else
                ++middle[resumed.bowler[i]];
        }
        assert(memcmp(observed, counts, sizeof(counts)) == 0);
        if (checkpoint)
            assert(resumed.bowler[checkpoint - 1] == last);
        for (i = 0; i < 6; ++i)
            assert(total[i] <= overs / 5);
        if (overs != 5) {
            assert(middle[0] == 0 && middle[1] == 0);
            for (i = 2; i < 5; ++i)
                assert(middle[i] >= (overs == 20 ? 2U : 1U) &&
                       middle[i] <= (overs == 20 ? 3U : 2U));
            assert(middle[5] >= 1 && middle[5] <= (overs == 20 ? 2U : 1U));
        }
        if (checkpoint < overs)
            ++counts[original.bowler[checkpoint]];
    }
}

static void check_resume_errors(void)
{
    c07_bowling_plan p, before;
    uint32_t state = 123;
    unsigned counts[6] = {1, 0, 0, 0, 0, 0};
    memset(&p, 0xa5, sizeof(p));
    before = p;
    /* Count says B1 bowled the first over, but last says B2. */
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 1, counts,
                                  1, &p) == C07_PLAN_NO_SCHEDULE);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    /* Individually plausible counts, but contrary to the fixed opening. */
    counts[0] = 0;
    counts[2] = 1;
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 1, counts,
                                  2, &p) == C07_PLAN_NO_SCHEDULE);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    /* Wrong last bowler with otherwise feasible prefix totals. */
    counts[0] = 1;
    counts[1] = 1;
    counts[2] = 0;
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 2, counts,
                                  0, &p) == C07_PLAN_NO_SCHEDULE);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    counts[0] = 5;
    counts[1] = 0;
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 5, counts,
                                  0, &p) == C07_PLAN_NO_SCHEDULE);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 21, counts,
                                  0, &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 1, 0,
                                  0, &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_make_bowling_plan_at(20, 2, random32, &state, 0, 0,
                                  0, &p) == C07_PLAN_INVALID_ARGUMENT);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
}

int main(void)
{
    unsigned i, counts[4] = {0};
    c07_bowling_plan out, before;
    uint32_t state = 1, count20, count10a, count10b;
    static const unsigned char five[] = {0, 2, 4, 3, 1};
    count20 = brute_count(20, 2, 0, 1, counts);
    count10a = brute_count(10, 2, 0, 0, counts);
    count10b = brute_count(10, 3, 0, 1, counts);
    assert(count20 == 10684 && count10a == 60 && count10b == 60);
    assert(c07_count_middle_sequences(20, 2) == count20);
    assert(c07_count_middle_sequences(10, 2) == count10a);
    assert(c07_count_middle_sequences(10, 3) == count10b);
    for (i = 1; i <= 10000; ++i) {
        check_plan(20, 2, i);
        check_plan(10, 2, i);
        check_plan(10, 3, i);
    }
    for (i = 1; i <= 5; ++i) {
        check_resume(20, 2, i);
        check_resume(10, 2, i);
        check_resume(10, 3, i);
        check_resume(5, 2, i);
    }
    check_resume_errors();
    assert(c07_make_bowling_plan(5, 999, 0, 0, &out) == C07_PLAN_OK);
    assert(out.overs == 5 && out.powerplay_overs == 1);
    assert(memcmp(out.bowler, five, sizeof(five)) == 0);
    memset(&out, 0x5a, sizeof(out));
    before = out;
    assert(c07_make_bowling_plan(30, 2, random32, &state, &out) == C07_PLAN_UNSUPPORTED);
    assert(memcmp(&out, &before, sizeof(out)) == 0);
    assert(c07_make_bowling_plan(10, 1, random32, &state, &out) == C07_PLAN_INVALID_ARGUMENT);
    assert(memcmp(&out, &before, sizeof(out)) == 0);
    assert(c07_make_bowling_plan(20, 2, 0, 0, &out) == C07_PLAN_INVALID_ARGUMENT);
    assert(memcmp(&out, &before, sizeof(out)) == 0);
    assert(c07_make_bowling_plan(20, 2, random32, &state, 0) == C07_PLAN_INVALID_ARGUMENT);
    assert(c07_count_middle_sequences(50, 2) == 0);
    assert(c07_count_middle_sequences(10, 1) == 0);
    printf("PASS: 30,000 seeded schedules, reproducibility, quotas, phases, adjacency, errors.\n");
    printf("PASS: 245 checkpoint resumes, including complete innings, and impossible histories.\n");
    printf("Complete feasible middles: T20=%lu T10/B3=%lu T10/B4=%lu\n",
           (unsigned long)count20, (unsigned long)count10a, (unsigned long)count10b);
    return 0;
}
