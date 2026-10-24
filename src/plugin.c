#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "schedule.h"
#include "hook.h"
#include "field_policy.h"

_Static_assert(sizeof(void *) == 4, "Cricket07 requires x86");
#define THISCALL __attribute__((thiscall))
typedef int (__cdecl *OpeningFn)(BYTE *);
typedef void (__cdecl *MatchFn)(BYTE *);

typedef int (__cdecl *MatchPredicateFn)(BYTE *);
typedef int (__stdcall *PowerplayFn)(int);
typedef BYTE *(__cdecl *ObjectFn)(void);
typedef void (THISCALL *RefreshFn)(BYTE *);
typedef uint32_t (THISCALL *ControllerFn)(BYTE *);
typedef int (__cdecl *AssignmentFn)(int,int);
typedef int (__cdecl *FieldSelectFn)(void);
typedef BYTE (THISCALL *MoveFieldFn)(BYTE *,int,BYTE *);
typedef BYTE (THISCALL *ValidateFieldFn)(BYTE *,BYTE *);
typedef BYTE (THISCALL *PresetFieldFn)(BYTE *,int,int,int);
typedef const char *(THISCALL *TextFn)(BYTE *,unsigned);
typedef void (THISCALL *PrepareFieldFn)(BYTE *);
typedef C07Vec3 *(THISCALL *GetPositionFn)(BYTE *,C07Vec3 *);
typedef void (THISCALL *SetPositionFn)(BYTE *,const C07Vec3 *);

static HMODULE module_self;
static BYTE *image_base;
static volatile LONG initialized, status;
static uint64_t random_state, variant_random_state;
static MatchFn swap_batters, swap_bowlers;
static ObjectFn get_world, get_world_again, get_ui;
static RefreshFn refresh_ui;
static ObjectFn get_bowling_controller;
static ControllerFn first_controller, second_controller;
static AssignmentFn get_assignment;
static ObjectFn get_match;
static GetPositionFn get_position;
static SetPositionFn set_position;
static MatchPredicateFn special_field_mode;
static int __cdecl opening_hook(BYTE *match);
static void __cdecl end_over_hook(BYTE *match);
static int __stdcall powerplay_hook(int overs);
static int __cdecl field_select_hook(void);
static BYTE THISCALL field_move_hook(BYTE *,int,BYTE *);
static BYTE THISCALL field_validate_hook(BYTE *,BYTE *);
static BYTE THISCALL field_preset_hook(BYTE *,int,int,int);
static const char *THISCALL field_text_hook(BYTE *,unsigned);
static void THISCALL field_prepare_hook(BYTE *);
static C07Hook hooks[C07_PLAN_HOOKS] = {
    {0x6f350,6,{0x53,0x56,0x8b,0x74,0x24,0x0c},(const void *)&opening_hook,NULL},
    {0x71cc0,5,{0x56,0x8b,0x74,0x24,0x08},(const void *)&end_over_hook,NULL},
    {0x8eef0,7,{0x8b,0x4c,0x24,0x04,0x8d,0x41,0xfb},(const void *)&powerplay_hook,NULL},
    {0x19f840,5,{0x51,0x53,0x55,0x56,0x57},(const void *)&field_select_hook,NULL},
    {0x91330,8,{0x83,0xec,0x10,0x53,0x8b,0x5c,0x24,0x18},(const void *)&field_move_hook,NULL},
    {0x914e0,6,{0x51,0x53,0x55,0x56,0x33,0xdb},(const void *)&field_validate_hook,NULL},
    {0x91280,7,{0x83,0xec,0x08,0x8d,0x44,0x24,0x04},(const void *)&field_preset_hook,NULL},
    {0x2e3610,7,{0x55,0x8b,0xec,0x51,0x89,0x4d,0xfc},(const void *)&field_text_hook,NULL},
    {0x1b2350,6,{0x81,0xec,0x38,0x02,0x00,0x00},(const void *)&field_prepare_hook,NULL}
};
static const BYTE expected_sha[32] = {
    0xd7,0x76,0xdb,0xa0,0xcc,0x50,0x6b,0x96,0x8f,0x7d,0x22,0x8e,0x03,0x6c,0xf4,0x2a,
    0xeb,0x4a,0xbe,0x45,0xa8,0xd1,0xad,0x0d,0x72,0x26,0xcd,0xeb,0xf9,0x15,0x53,0xa2
};
typedef struct PlanCache {
    int valid;
    BYTE *match;
    uint32_t team;
    unsigned innings, overs, size;
    uint16_t ids[6];
    c07_bowling_plan plan;
} PlanCache;
static PlanCache cache[4];
typedef struct FieldVariantCache {
    int valid;
    BYTE *match, *batting;
    uint32_t team;
    unsigned overs, fours, sixes, wickets, variant;
} FieldVariantCache;
static FieldVariantCache field_variants[4];

static uint16_t read16(const void *p) { uint16_t v; memcpy(&v,p,2); return v; }
static uint32_t read32(const void *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void write32(void *p,uint32_t v) { memcpy(p,&v,4); }
static BYTE *read_pointer(const void *p) { return (BYTE *)(uintptr_t)read32(p); }
static void write_pointer(void *p,BYTE *v) { write32(p,(uint32_t)(uintptr_t)v); }
static void *address(uint32_t va) { return image_base+(va-0x400000u); }
static int supported_length(unsigned n) { return n==5 || n==10 || n==20 || n==40 || n==45 || n==50; }
static int cpu_bowling(const BYTE *m) {
    BYTE *controller;
    /* Exact inverse of the human guard at 471CC0. A slot byte alone does not
       establish a human: unassigned CPU sides can have slot 0 and null actors. */
    if (read32(m+0x211f4)!=0 || m[0x2120d]==0xff) return 1;
    controller=get_bowling_controller();
    return !controller || !first_controller(controller) || !second_controller(controller);
}
static int cpu_opening(const BYTE *m) {
    int side;
    uint32_t bowling=read32(m+0xd568);
    if (read32(m+0x211f4)!=0 || m[0x2120d]==0xff) return 1;
    /* Controller actors and frontend CPU flags are rebuilt AFTER 46F350.
       Read the current bowling team's device assignments instead. */
    if (bowling==read32(m+0x1c20)) side=0;
    else if (bowling==read32(m+0x4858)) side=1;
    else return 0;
    return get_assignment(side,0)==-1 && get_assignment(side,1)==-1;
}
static unsigned completed_overs(const BYTE *m) {
    /* Quick simulation updates the innings counter, not the interactive
       global counter at 58C6. Both paths update 58E4 before end-over selection. */
    unsigned innings=m[0x58c9];
    if (read32(m+0x211f4)!=0 && innings<4) return read16(m+0x58e4+2*innings);
    return read16(m+0x58c6);
}

static void log_line(const char *line) {
#ifdef C07_TEST
    (void)line;
#else
    WCHAR path[MAX_PATH];
    DWORD n=GetModuleFileNameW(module_self,path,MAX_PATH), written;
    HANDLE file;
    if (!n || n>=MAX_PATH) return;
    while (n && path[n-1]!=L'\\') --n;
    if (n+32>=MAX_PATH) return;
    memcpy(path+n,L"Cricket07BowlingPlan.log",sizeof(L"Cricket07BowlingPlan.log"));
    file=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
                     OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file==INVALID_HANDLE_VALUE) return;
    WriteFile(file,line,(DWORD)strlen(line),&written,NULL);
    WriteFile(file,"\r\n",2,&written,NULL);
    CloseHandle(file);
#endif
}
static void log_state(const char *event,BYTE *m,int cpu) {
    char line[320];
    BYTE *current=read_pointer(m+0xd540),*other=read_pointer(m+0xd544);
    snprintf(line,sizeof(line),"%s innings=%u overs=%u completed=%u mode=%lu slot=%u cpu=%d current=%u other=%u",
             event,m[0x58c9]+1,read16(m+0x58c4),completed_overs(m),
             (unsigned long)read32(m+0x211f4),m[0x2120d],cpu,
             current?read16(current):0,other?read16(other):0);
    log_line(line);
}

static uint32_t random32(void *unused) {
    /* PCG-XSH-RR; independent of the game's RNG and shot/bowling random draws. */
    uint64_t old=random_state;
    uint32_t value, rotation;
    (void)unused;
    random_state=old*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
    value=(uint32_t)(((old>>18)^old)>>27);
    rotation=(uint32_t)(old>>59);
    return (value>>rotation)|(value<<((-rotation)&31));
}

static unsigned next_field_variant(unsigned current) {
    /* Independent PCG stream: field changes must not consume scheduling RNG.
       Four equally sized outcomes, mapped around the current zero-based index. */
    uint64_t old=variant_random_state;
    uint32_t value,rotation,choice;
    variant_random_state=old*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
    value=(uint32_t)(((old>>18)^old)>>27); rotation=(uint32_t)(old>>59);
    choice=((value>>rotation)|(value<<((-rotation)&31)))&3u;
    return choice>=current?choice+1:choice;
}

static int read_pool(BYTE *m,unsigned n,BYTE *pool[6],uint16_t ids[6]) {
    for (unsigned i=0;i<n;++i) {
        pool[i]=read_pointer(m+0xd548+4*i);
        if (!pool[i] || !(ids[i]=read16(pool[i])) || (read32(pool[i]+0x8c)&63)==4) return 0;
        for (unsigned j=0;j<i;++j) if (ids[j]==ids[i]) return 0;
    }
    return 1;
}
static unsigned pool_index(BYTE *p,BYTE *const pool[6],unsigned n) {
    for (unsigned i=0;i<n;++i) if (p==pool[i]) return i;
    return 6;
}

static void log_plan(const PlanCache *c,BYTE *const pool[6],unsigned completed) {
    char line[800];
    int used=snprintf(line,sizeof(line),"PLAN innings=%u team=%lu overs=%u resume=%u:",
                      c->innings+1,(unsigned long)c->team,c->overs,completed);
    for (unsigned i=completed;i<c->overs;++i)
        used+=snprintf(line+used,sizeof(line)-(size_t)used," %u:B%u",i+1,c->plan.bowler[i]+1);
    log_line(line);
    for (unsigned i=0;i<c->size;++i) {
        snprintf(line,sizeof(line),"B%u = player %u (%.59s)",i+1,c->ids[i],(char *)pool[i]+4);
        log_line(line);
    }
}

static PlanCache *obtain_plan(BYTE *m,BYTE *pool[6],int new_opening) {
    unsigned n=read16(m+0x58c4), completed=completed_overs(m), innings=m[0x58c9];
    unsigned size=n==5?5:6, counts[6]={0}, last=6;
    uint16_t ids[6]={0};
    PlanCache *c;
    int same;
    if (!supported_length(n) || completed>n || innings>=4 || !read_pool(m,size,pool,ids)) return NULL;
    c=&cache[innings];
    same=c->valid && c->match==m && c->innings==innings && c->overs==n &&
         c->team==read32(m+0xd568) && !memcmp(c->ids,ids,sizeof(ids));
    if (completed) {
        for (unsigned i=0;i<size;++i) counts[i]=read32(pool[i]+0xa8)&0x7ff;
        last=pool_index(read_pointer(m+0xd540),pool,size);
    }
    if (same && !(new_opening && !completed)) {
        unsigned expected[6]={0};
        for (unsigned i=0;i<completed;++i) ++expected[c->plan.bowler[i]];
        if (!memcmp(counts,expected,sizeof(counts)) &&
            (!completed || last==c->plan.bowler[completed-1])) return c;
    }
    c->valid=0;
    if (c07_make_bowling_plan_at(n,2,random32,NULL,completed,counts,last,&c->plan)!=C07_PLAN_OK) return NULL;
    c->match=m; c->team=read32(m+0xd568); c->innings=innings; c->overs=n; c->size=size;
    memcpy(c->ids,ids,sizeof(ids)); c->valid=1;
    log_plan(c,pool,completed);
    return c;
}

static void clear_bowling_role(BYTE *p) {
    uint32_t bits;
    if (!p) return;
    bits=read32(p+0x8c);
    if ((bits&63)==1 || (bits&63)==2) write32(p+0x8c,bits&~63u);
}
static void set_bowling_role(BYTE *p,unsigned role) {
    write32(p+0x8c,(read32(p+0x8c)&~63u)|role);
}

static int __cdecl opening_hook(BYTE *m) {
    /* Reset even when a new match reuses the same native match/record memory.
       Do this before the original in case it prepares the opening field. */
    unsigned innings=m[0x58c9];
    if (innings<4 && supported_length(read16(m+0x58c4)))
        memset(&field_variants[innings],0,sizeof(field_variants[innings]));
    int result=((OpeningFn)hooks[0].trampoline)(m);
    int cpu=cpu_opening(m);
    BYTE *pool[6]={0}, *current, *other;
    PlanCache *c;
    log_state("OPEN",m,cpu);
    if (!cpu || !supported_length(read16(m+0x58c4))) return result;
    c=obtain_plan(m,pool,1);
    if (!c || completed_overs(m)!=0) {
        log_line("FALLBACK opening: invalid pool/history; native opening retained.");
        return result;
    }
    current=pool[c->plan.bowler[0]]; other=pool[c->plan.bowler[1]];
    clear_bowling_role(read_pointer(m+0xd540));
    clear_bowling_role(read_pointer(m+0xd544));
    for (unsigned i=0;i<c->size;++i) clear_bowling_role(pool[i]);
    write_pointer(m+0xd540,current); write_pointer(m+0xd544,other);
    set_bowling_role(current,1); set_bowling_role(other,2);
    log_state("OPEN APPLIED",m,cpu);
    return result;
}

static void native_end_tail(BYTE *m) {
    swap_batters(m);
    swap_bowlers(m);
    if (get_world() && read32(m+0x211f4)==2) refresh_ui(get_ui());
}
static void __cdecl end_over_hook(BYTE *m) {
    unsigned n=read16(m+0x58c4), completed=completed_overs(m);
    int cpu=cpu_bowling(m);
    BYTE *pool[6]={0};
    PlanCache *c;
    log_state("END",m,cpu);
    if (!cpu || !supported_length(n)) goto original;
    c=obtain_plan(m,pool,0);
    if (!c) {
        log_line("FALLBACK end over: invalid/incompatible bowling pool or history; native chooser used.");
        goto original;
    }
    if (completed<n) {
        BYTE *next=pool[c->plan.bowler[completed]];
        BYTE *old=read_pointer(m+0xd544);
        if (next!=old) {
            clear_bowling_role(old);
            write_pointer(m+0xd544,next);
            if (get_world()) {
                BYTE *world=get_world_again();
                if (world) world[0x65]=1;
            }
        }
    }
    native_end_tail(m);
    log_state("NEXT APPLIED",m,cpu);
    return;
original:
    ((MatchFn)hooks[1].trampoline)(m);
}

static int __stdcall powerplay_hook(int overs) {
    if (overs==5) return 1;
    if (overs==10) return 3;
    if (overs==20) return 6;
    if (overs==40 || overs==45 || overs==50) return overs/5;
    return ((PowerplayFn)hooks[2].trampoline)(overs);
}

static int current_policy(C07FieldPolicy *p) {
    BYTE *m=get_match();
    return m && C07FieldPolicyFor(read16(m+0x58c4),completed_overs(m),p);
}
static int automatic_field(BYTE *manager) {
    BYTE owner,*record;
    if (!manager) return 0;
    owner=manager[5];
    if (owner==0xff) return 1;
    record=read_pointer(manager+0x44+4*owner);
    return !record || !read_pointer(record+4);
}
static unsigned field_family(int local) {
    return local<10?C07_DEFEND:local<20?C07_NORMAL:C07_OPEN;
}
static unsigned current_striker_runs(BYTE *match) {
    BYTE *striker=read_pointer(match+0xd538);
    /* Same packed innings-runs field read by native selector 59F840. */
    return striker?(read32(striker+0xa4)>>20)&0x7ffu:0;
}
static unsigned current_field_variant(BYTE *m) {
    unsigned innings=m[0x58c9],fours=0,sixes=0,wickets,n=read16(m+0x58c4);
    BYTE *batting=read_pointer(m+0xd530);
    uint32_t team=read32(m+0xd568);
    FieldVariantCache *c;
    int raw_wickets;
    if (innings>=4 || !batting) return 1; /* XML variant 2; wait for valid records. */
    /* Match-local batting XI, including dismissed players. Native updates at
       472E71/473B57 increment A4 bits10..19 for fours or bits0..9 for sixes.
       Runs are bits20..30 and are deliberately ignored here. */
    for (unsigned i=0;i<11;++i) {
        uint32_t bits=read32(batting+i*0x168+0xa4);
        fours+=(bits>>10)&0x3ffu; sixes+=bits&0x3ffu;
    }
    raw_wickets=(int16_t)read16(m+0x5910+2*innings);
    wickets=(unsigned)(raw_wickets<0?-1-raw_wickets:raw_wickets);
    if (wickets>10) return 1;
    c=&field_variants[innings];
    if (!c->valid || c->match!=m || c->batting!=batting || c->team!=team || c->overs!=n ||
        fours<c->fours || sixes<c->sixes || wickets<c->wickets) {
        /* A resumed innings starts at 2 using its current counters as baseline;
           historical boundaries are not replayed as new events. */
        c->valid=1; c->match=m; c->batting=batting; c->team=team; c->overs=n; c->variant=1;
    } else if (fours>c->fours || sixes>c->sixes || wickets>c->wickets) {
        char line[180];
        unsigned previous=c->variant;
        c->variant=next_field_variant(previous);
        snprintf(line,sizeof(line),"FIELD VARIANT innings=%u %u->%u fours=%u->%u sixes=%u->%u wickets=%u->%u",
                 innings+1,previous+1,c->variant+1,c->fours,fours,c->sixes,sixes,c->wickets,wickets);
        log_line(line);
    }
    c->fours=fours; c->sixes=sixes; c->wickets=wickets;
    return c->variant;
}
static int __cdecl field_select_hook(void) {
    int original=((FieldSelectFn)hooks[3].trampoline)();
    int base,local,variant,offset,result;
    BYTE *m=get_match(),*striker;
    unsigned choices[3],count=0,family,allowed,runs,over,n;
    C07FieldPolicy p;
    int circle_policy,check_runs;
    if (!m || !automatic_field(get_bowling_controller())) return original;
    n=read16(m+0x58c4); over=completed_overs(m); check_runs=n==10 || n==20;
    runs=check_runs?current_striker_runs(m):0;
    circle_policy=current_policy(&p);
    if (!C07FieldSelectionFor(n,over,runs,&allowed)) {
        if (!circle_policy) return original;
        allowed=p.allowed;
    }
    if (original>=140 && original<170) base=140;
    else if (original>=35 && original<65) base=35;
    else if (original>=0 && original<30) base=0;
    else { log_line("FIELD FALLBACK: unrecognized ODI field bank."); return original; }
    local=original-base; variant=(int)current_field_variant(m); family=field_family(local);
    if (!(allowed&family)) {
        if (allowed&C07_OPEN) choices[count++]=C07_OPEN;
        if (allowed&C07_NORMAL) choices[count++]=C07_NORMAL;
        if (allowed&C07_DEFEND) choices[count++]=C07_DEFEND;
        family=count==1?choices[0]:choices[random32(NULL)%count];
    }
    /* Preserve the native bowling bank and the existing family policy.
       Variant 2 starts each innings; only boundaries/wickets reshuffle 1..5. */
    offset=family==C07_OPEN?20:family==C07_NORMAL?10:0;
    result=base+offset+variant;
    striker=read_pointer(m+0xd538);
    {
        static BYTE *last_match;
        static unsigned last_inning,last_over,last_runs,last_striker;
        static int last_original,last_result;
        unsigned id=striker?read16(striker):0,inning=m[0x58c9];
        if (m!=last_match || inning!=last_inning || over!=last_over || runs!=last_runs ||
            id!=last_striker || original!=last_original || result!=last_result) {
            char line[220],run_text[16];
            unsigned limit=circle_policy?p.outside_limit:(over<(unsigned)powerplay_hook((int)n)?2u:5u);
            if (check_runs) snprintf(run_text,sizeof(run_text),"%u",runs);
            else memcpy(run_text,"ignored",sizeof("ignored"));
            snprintf(line,sizeof(line),"FIELD SELECT overs=%u over=%u striker=%u runs=%s family=%s max=%u preset=%d->%d",
                     n,over+1,id,run_text,family==C07_OPEN?"Open":family==C07_NORMAL?"Normal":"Defend",limit,original,result);
            log_line(line);
            last_match=m; last_inning=inning; last_over=over; last_runs=runs;
            last_striker=id; last_original=original; last_result=result;
        }
    }
    return result;
}
static int in_range(uint32_t c,uint32_t a,uint32_t b) { return c>=a && c<=b; }
static int category_outside(BYTE *c,uint32_t v) {
    return in_range(v,read32(c+0x2a78),read32(c+0x2a8c)) ||
           in_range(v,read32(c+0x2a7c),read32(c+0x2a90));
}
static int category_restricted(BYTE *c,uint32_t v) {
    return category_outside(c,v) || in_range(v,read32(c+0x2a78),15) ||
           in_range(v,read32(c+0x2a7c),34) || in_range(v,read32(c+0x2a80),52) ||
           in_range(v,read32(c+0x2a84),73) || in_range(v,read32(c+0x2a88),84);
}
static unsigned outside_rows(BYTE *c,int proposed,int moving) {
    unsigned total=0;
    for (int i=0;i<88;++i) {
        BYTE *row=c+0x10+0x50*i;
        if (i!=proposed && row[0]==0xff) continue;
        if (moving && (int)(int8_t)row[0]==(int)c[0x1b80]) continue;
        total+=(unsigned)category_outside(c,(uint32_t)(int32_t)(int8_t)row[1]);
    }
    return total;
}
static BYTE THISCALL field_move_hook(BYTE *c,int proposed,BYTE *reason) {
    BYTE result=((MoveFieldFn)hooks[4].trampoline)(c,proposed,reason);
    C07FieldPolicy p;
    if (!result || !current_policy(&p) || p.outside_limit!=4 || proposed<0 || proposed>=88) return result;
    if (category_restricted(c,(uint32_t)(int32_t)(int8_t)c[0x11+0x50*proposed]) &&
        outside_rows(c,proposed,1)>4) { if (reason) *reason=2; return 0; }
    return result;
}
static BYTE THISCALL field_validate_hook(BYTE *c,BYTE *reason) {
    BYTE result=((ValidateFieldFn)hooks[5].trampoline)(c,reason);
    C07FieldPolicy p;
    if (result && current_policy(&p) && p.outside_limit==4 && outside_rows(c,-1,0)>4) {
        if (reason) *reason=2;
        return 0;
    }
    return result;
}
static BYTE THISCALL field_preset_hook(BYTE *c,int index,int limited,int pp) {
    BYTE result=((PresetFieldFn)hooks[6].trampoline)(c,index,limited,pp);
    C07FieldPolicy p;
    unsigned outside=0;
    if (!result || !(limited&255) || !current_policy(&p) || p.outside_limit!=4 || index<0 || index>64) return result;
    for (unsigned i=0;i<9;++i) outside+=(unsigned)category_outside(c,c[0x282d+9*index+i]);
    return outside>4?0:result;
}
static const char *THISCALL field_text_hook(BYTE *self,unsigned id) {
    C07FieldPolicy p;
    if (id==0x20e && current_policy(&p) && p.outside_limit==4)
        return "No more than four fielders are allowed outside the circle during Powerplay 2.";
    return ((TextFn)hooks[7].trampoline)(self,id);
}
static void THISCALL field_prepare_hook(BYTE *manager) {
    C07FieldPolicy p;
    C07Vec3 positions[9]={{0}};
    BYTE *actors[9]={0};
    unsigned valid=0,moved=0,before,after;
    ((PrepareFieldFn)hooks[8].trampoline)(manager);
    /* Native practice mode parks actors away from the playable field. Preserve
       that placement instead of pulling parked actors into the circle. */
    if (special_field_mode(get_match())) return;
    if (!current_policy(&p) || !automatic_field(manager)) return;
    for (unsigned i=0;i<9;++i) {
        actors[i]=read_pointer(manager+0x84+4*i);
        if (actors[i]) { get_position(actors[i],&positions[i]); valid|=1u<<i; }
    }
    before=C07OutsideCount(positions,valid);
    C07LimitOutside(positions,valid,p.outside_limit,&moved);
    for (unsigned i=0;i<9;++i) if (moved&(1u<<i)) set_position(actors[i],&positions[i]);
    after=C07OutsideCount(positions,valid);
    {
        static BYTE *last_match;
        static unsigned last_inning,last_over,last_phase;
        BYTE *m=get_match();
        unsigned over=completed_overs(m),inning=m[0x58c9];
        if (moved || m!=last_match || inning!=last_inning || over!=last_over || p.phase!=last_phase) {
            char line[180];
            snprintf(line,sizeof(line),"FIELD APPLIED over=%u phase=%u max=%u outside=%u->%u moved_mask=%03x",
                     over+1,p.phase,p.outside_limit,before,after,moved);
            log_line(line);
            last_match=m; last_inning=inning; last_over=over; last_phase=p.phase;
        }
    }
}

/* 1 = known reference hash, 0 = different hash, -1 = cannot calculate.
   This is diagnostic only; PE shape and instruction signatures gate hooks. */
static int executable_hash(const WCHAR *path,char actual[65]) {
    BYTE buffer[16384],digest[32];
    DWORD n,size=sizeof(digest);
    HANDLE file;
    HCRYPTPROV provider=0;
    HCRYPTHASH hash=0;
    int result=-1;
    actual[0]=0;
    file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                     NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file==INVALID_HANDLE_VALUE) return -1;
    if (!CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) goto done;
    for (;;) {
        if (!ReadFile(file,buffer,sizeof(buffer),&n,NULL)) goto done;
        if (!n) break;
        if (!CryptHashData(hash,buffer,n,0)) goto done;
    }
    if (!CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0) || size!=sizeof(digest)) goto done;
    for (unsigned i=0;i<sizeof(digest);++i) snprintf(actual+2*i,3,"%02x",digest[i]);
    result=!memcmp(digest,expected_sha,sizeof(digest));
done:
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider,0);
    CloseHandle(file);
    return result;
}

static void log_executable_hash(void) {
    WCHAR path[MAX_PATH];
    char actual[65],line[200];
    DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    int result=length && length<MAX_PATH?executable_hash(path,actual):-1;
    if (result<0) log_line("WARNING: executable SHA-256 unavailable; continuing with PE/layout and hook-signature checks.");
    else {
        snprintf(line,sizeof(line),result?
                 "Executable SHA-256 matches reference: %s":
                 "WARNING: executable SHA-256 differs from reference: %s; continuing with PE/layout and hook-signature checks.",actual);
        log_line(line);
    }
}

static void initialize_image(BYTE *image) {
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS32 *pe;
    HMODULE pinned;
    HCRYPTPROV provider=0;
    LARGE_INTEGER counter;
    log_line("Cricket07BowlingPlan 2.5 initializing (SHA-256 diagnostic only; existing gameplay rules retained).");
    log_executable_hash();
    image_base=image;
    if (!image_base) goto unsupported;
    dos=(IMAGE_DOS_HEADER *)image_base;
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE) goto unsupported;
    pe=(IMAGE_NT_HEADERS32 *)(image_base+dos->e_lfanew);
    if (pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
        pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        pe->OptionalHeader.SizeOfImage!=6606446u) goto unsupported;
    QueryPerformanceCounter(&counter);
    random_state=(uint64_t)counter.QuadPart^((uint64_t)GetCurrentProcessId()<<32)^GetTickCount();
    if (CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(provider,sizeof(random_state),(BYTE *)&random_state);
        CryptReleaseContext(provider,0);
    }
    (void)random32(NULL);
    variant_random_state=random_state^UINT64_C(0xda3e39cb94b95bdb);
    swap_batters=(MatchFn)address(0x46fed0);
    swap_bowlers=(MatchFn)address(0x4628a0);
    get_world=(ObjectFn)address(0x403520);
    get_world_again=(ObjectFn)address(0x403ce0);
    get_ui=(ObjectFn)address(0x413b80);
    refresh_ui=(RefreshFn)address(0x415330);
    get_bowling_controller=(ObjectFn)address(0x5a4ac0);
    first_controller=(ControllerFn)address(0x5a6c10);
    second_controller=(ControllerFn)address(0x5a6bf0);
    get_assignment=(AssignmentFn)address(0x487e10);
    get_match=(ObjectFn)address(0x4ec220);
    get_position=(GetPositionFn)address(0x42c500);
    set_position=(SetPositionFn)address(0x5a34f0);
    special_field_mode=(MatchPredicateFn)address(0x462360);
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
                            (LPCWSTR)(uintptr_t)&opening_hook,&pinned) ||
        !C07InstallPlanHooks(image_base,hooks)) {
        status=-2;
        log_line("NOT APPLIED: entry signature conflict or hook installation failure.");
        return;
    }
    status=1;
    log_line("APPLIED: rotation/PP, ODI field selector, three legality checks, phase warning and final field placement (9 hooks).");
    return;
unsupported:
    status=-1;
    log_line("NOT APPLIED: unsupported executable PE image/layout.");
}
void InitializeASI(void) {
    if (InterlockedCompareExchange(&initialized,1,0)) return;
    initialize_image((BYTE *)GetModuleHandleW(NULL));
}
LONG GetBowlingPlanStatus(void) { return InterlockedCompareExchange(&status,0,0); }
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason==DLL_PROCESS_ATTACH) { module_self=module; DisableThreadLibraryCalls(module); }
    return TRUE;
}
