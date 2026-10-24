#include "field_policy.h"
#include <math.h>
int C07FieldSelectionFor(unsigned overs,unsigned completed,unsigned runs,unsigned *allowed) {
    unsigned pp,cutoff,run_limit;
    if (!allowed) return 0;
    switch (overs) {
    case 5: pp=1; cutoff=1; run_limit=0; break;
    case 10: pp=3; cutoff=6; run_limit=20; break;
    case 20: pp=6; cutoff=12; run_limit=30; break;
    case 40: pp=8; cutoff=32; run_limit=0; break;
    case 45: pp=9; cutoff=36; run_limit=0; break;
    case 50: pp=10; cutoff=40; run_limit=0; break;
    default: return 0;
    }
    if (completed<pp) *allowed=C07_OPEN;
    else if (completed>=cutoff || ((overs==10 || overs==20) && runs>run_limit)) *allowed=C07_DEFEND;
    else *allowed=C07_NORMAL;
    return 1;
}
int C07FieldPolicyFor(unsigned overs,unsigned completed,C07FieldPolicy *out) {
    C07FieldPolicy p;
    unsigned fifth=overs/5;
    if (!out || (overs!=40 && overs!=45 && overs!=50)) return 0;
    if (completed<fifth) {
        p.phase=1; p.outside_limit=2; p.allowed=C07_OPEN;
    } else if (completed<2*fifth) {
        p.phase=2; p.outside_limit=4; p.allowed=C07_NORMAL;
    } else if (completed<4*fifth) {
        p.phase=2; p.outside_limit=4; p.allowed=C07_OPEN|C07_NORMAL;
    } else {
        p.phase=3; p.outside_limit=5; p.allowed=C07_OPEN|C07_NORMAL|C07_DEFEND;
    }
    *out=p;
    return 1;
}
static float axis_z(float z) {
    return z>10.06f?10.06f:z < -10.06f?-10.06f:z;
}
static float distance_squared(C07Vec3 p) {
    float dz=p.z-axis_z(p.z);
    if (!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z)) return 0;
    return p.x*p.x+dz*dz;
}
unsigned C07OutsideCount(const C07Vec3 p[9],unsigned valid) {
    unsigned count=0;
    for (unsigned i=0;i<9;++i)
        if ((valid&(1u<<i)) && distance_squared(p[i])>27.432f*27.432f) ++count;
    return count;
}
unsigned C07LimitOutside(C07Vec3 p[9],unsigned valid,unsigned limit,unsigned *mask) {
    unsigned outside=C07OutsideCount(p,valid),moved=0;
    *mask=0;
    while (outside>limit) {
        unsigned best=9;
        float nearest=INFINITY;
        for (unsigned i=0;i<9;++i) {
            float d=distance_squared(p[i]);
            if ((valid&(1u<<i)) && d>27.432f*27.432f && d<nearest) { best=i; nearest=d; }
        }
        if (best==9) break;
        {
            float z0=axis_z(p[best].z),scale=26.5f/sqrtf(nearest);
            p[best].x*=scale;
            p[best].z=z0+(p[best].z-z0)*scale;
        }
        *mask|=1u<<best; ++moved; --outside;
    }
    return moved;
}
