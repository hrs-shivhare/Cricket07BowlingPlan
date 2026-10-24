#ifndef CRICKET07_FIELD_POLICY_H
#define CRICKET07_FIELD_POLICY_H
enum { C07_OPEN=1, C07_NORMAL=2, C07_DEFEND=4 };
typedef struct C07FieldPolicy {
    unsigned phase;
    unsigned outside_limit;
    unsigned allowed;
} C07FieldPolicy;
typedef struct C07Vec3 { float x,y,z; } C07Vec3;
/* completed=0 describes over 1. Returns zero for untouched formats. */
int C07FieldPolicyFor(unsigned overs,unsigned completed,C07FieldPolicy *out);
/* Explicit CPU family thresholds. Only 10/20 overs use striker_runs;
   5/40/45/50 depend on the completed-over counter alone. */
int C07FieldSelectionFor(unsigned overs,unsigned completed,unsigned striker_runs,
                         unsigned *allowed);
/* Custom physical enforcement uses the 30-yard oval about the two wickets.
   Returns the number moved, preserving Y and moving the nearest excess first. */
unsigned C07LimitOutside(C07Vec3 positions[9],unsigned valid_mask,unsigned limit,
                         unsigned *moved_mask);
unsigned C07OutsideCount(const C07Vec3 positions[9],unsigned valid_mask);
#endif
