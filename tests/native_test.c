#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#define C07_TEST 1
#include "../src/plugin.c"

static BYTE match_data[0x21300], records[6][0x168], world[0x100], ui[8];
static BYTE batting_records[4][11][0x168];
static unsigned opening_calls, end_calls, batter_swaps, bowler_swaps, refreshes;
static int world_present;
static BYTE controller_object[0x200];
static uint32_t controller_values[2];
static int assignments[2][2];
static BYTE field_manager[0x200],field_config[0x2b00],field_actors[11][16];
static C07Vec3 pending_positions[11];
static int native_field_id;
static BYTE field_result,field_reason;
static unsigned field_prepares,position_sets;
static int practice_field_mode;
static BYTE *__cdecl mock_match(void) { return match_data; }
static int __cdecl mock_special_field(BYTE *m) { assert(m==match_data); return practice_field_mode; }
static int __cdecl mock_select(void) { return native_field_id; }
static BYTE THISCALL mock_move(BYTE *c,int pos,BYTE *r) { assert(c==field_config); (void)pos; *r=field_reason; return field_result; }
static BYTE THISCALL mock_validate(BYTE *c,BYTE *r) { assert(c==field_config); *r=field_reason; return field_result; }
static BYTE THISCALL mock_preset(BYTE *c,int i,int limited,int pp) { assert(c==field_config); (void)i; (void)limited; (void)pp; return field_result; }
static const char *THISCALL mock_text(BYTE *c,unsigned id) { (void)c; (void)id; return "native text"; }
static void THISCALL mock_prepare(BYTE *c) {
    assert(c==field_manager); ++field_prepares;
    for (unsigned i=0;i<11;++i) memcpy(field_actors[i],&pending_positions[i],sizeof(C07Vec3));
}
static C07Vec3 *THISCALL mock_get_position(BYTE *a,C07Vec3 *out) { memcpy(out,a,sizeof(*out)); return out; }
static void THISCALL mock_set_position(BYTE *a,const C07Vec3 *p) { memcpy(a,p,sizeof(*p)); ++position_sets; }
static BYTE *__cdecl mock_controller(void) { return controller_object; }
static uint32_t THISCALL mock_first(BYTE *p) { assert(p==controller_object); return controller_values[0]; }
static uint32_t THISCALL mock_second(BYTE *p) { assert(p==controller_object); return controller_values[1]; }
static int __cdecl mock_assignment(int side,int index) {
    assert(side>=0 && side<2 && index>=0 && index<2);
    return assignments[side][index];
}
static void put16(void *p,uint16_t v) { memcpy(p,&v,2); }
static int __cdecl mock_opening(BYTE *m) {
    ++opening_calls;
    write_pointer(m+0xd540,records[4]); write_pointer(m+0xd544,records[5]);
    set_bowling_role(records[4],1); set_bowling_role(records[5],2);
    return 37;
}
static void __cdecl mock_end(BYTE *m) { (void)m; ++end_calls; }
static int __stdcall mock_powerplay(int n) { return n+100; }
static BYTE *__cdecl mock_world(void) { return world_present?world:NULL; }
static BYTE *__cdecl mock_ui(void) { return ui; }
static void THISCALL mock_refresh(BYTE *p) { assert(p==ui); ++refreshes; }
static void __cdecl mock_batters(BYTE *m) { assert(m==match_data); ++batter_swaps; }
static void __cdecl mock_bowlers(BYTE *m) {
    BYTE *current=read_pointer(m+0xd540),*other=read_pointer(m+0xd544);
    assert(current!=other);
    write_pointer(m+0xd540,other); write_pointer(m+0xd544,current);
    set_bowling_role(other,1); set_bowling_role(current,2);
    ++bowler_swaps;
}
static void fixture(unsigned n) {
    memset(match_data,0,sizeof(match_data)); memset(records,0,sizeof(records));
    memset(batting_records,0,sizeof(batting_records)); memset(field_variants,0,sizeof(field_variants));
    memset(cache,0,sizeof(cache)); memset(world,0,sizeof(world));
    opening_calls=end_calls=batter_swaps=bowler_swaps=refreshes=0;
    world_present=1; random_state=UINT64_C(0x123456789abcdef);
    variant_random_state=UINT64_C(0xfedcba987654321);
    put16(match_data+0x58c4,(uint16_t)n);
    match_data[0x2120d]=0xff;
    write32(match_data+0xd568,12);
    write32(match_data+0x1c20,12); write32(match_data+0x4858,13);
    write_pointer(match_data+0xd530,batting_records[0][0]);
    assignments[0][0]=assignments[0][1]=assignments[1][0]=assignments[1][1]=-1;
    controller_values[0]=controller_values[1]=1;
    memset(controller_object,0,sizeof(controller_object)); controller_object[5]=0xff;
    for (unsigned i=0;i<6;++i) {
        put16(records[i],(uint16_t)(100+i));
        snprintf((char *)records[i]+4,60,"Fixture bowler %u",i+1);
        write32(records[i]+0x8c,0xa5000000u);
        write32(records[i]+0xa8,0xabc00000u);
        write_pointer(match_data+0xd548+4*i,records[i]);
    }
    hooks[0].trampoline=(void *)&mock_opening;
    hooks[1].trampoline=(void *)&mock_end;
    hooks[2].trampoline=(void *)&mock_powerplay;
    swap_batters=mock_batters; swap_bowlers=mock_bowlers;
    get_world=get_world_again=mock_world; get_ui=mock_ui; refresh_ui=mock_refresh;
    get_bowling_controller=mock_controller; first_controller=mock_first; second_controller=mock_second;
    get_assignment=mock_assignment;
    get_match=mock_match; get_position=mock_get_position; set_position=mock_set_position;
    special_field_mode=mock_special_field; practice_field_mode=0;
    hooks[3].trampoline=(void *)&mock_select;
    hooks[4].trampoline=(void *)&mock_move;
    hooks[5].trampoline=(void *)&mock_validate;
    hooks[6].trampoline=(void *)&mock_preset;
    hooks[7].trampoline=(void *)&mock_text;
    hooks[8].trampoline=(void *)&mock_prepare;
    memset(field_manager,0,sizeof(field_manager)); memset(field_config,0,sizeof(field_config));
    field_manager[5]=0xff; field_result=1; field_reason=1;
    field_prepares=position_sets=0; native_field_id=39;
    for (unsigned i=0;i<88;++i) { field_config[0x10+0x50*i]=0xff; field_config[0x11+0x50*i]=(BYTE)i; }
    field_config[0x1b80]=9;
    {
        const unsigned start[]={0,31,38,68,82},end[]={30,37,67,81,87};
        for (unsigned i=0;i<5;++i) { write32(field_config+0x2a78+4*i,start[i]); write32(field_config+0x2a8c+4*i,end[i]); }
    }
    for (unsigned i=0;i<11;++i) {
        write_pointer(field_manager+0x84+4*i,field_actors[i]);
        pending_positions[i]=(C07Vec3){60.0f+(float)i,(float)i,0};
    }
}
static unsigned index_of(BYTE *p) {
    for (unsigned i=0;i<6;++i) if (p==records[i]) return i;
    assert(0); return 6;
}
static void finish_over(unsigned completed) {
    BYTE *p=read_pointer(match_data+0xd540);
    /* The delivery update 472690 records the bowler here before end-over.
       D560 is a previous/current-bowler pointer, not a wicketkeeper pointer. */
    write_pointer(match_data+0xd560,p);
    write32(p+0xa8,read32(p+0xa8)+1);
    put16(match_data+0x58e4+2*match_data[0x58c9],(uint16_t)completed);
    /* Quick simulation leaves the interactive global counter stale. */
    if (!read32(match_data+0x211f4)) put16(match_data+0x58c6,(uint16_t)completed);
    end_over_hook(match_data);
}
static void check_totals(unsigned n) {
    unsigned sum=0;
    for (unsigned i=0;i<6;++i) {
        unsigned count=read32(records[i]+0xa8)&0x7ff;
        assert(count<=n/5); sum+=count;
        assert((read32(records[i]+0x8c)&~63u)==0xa5000000u);
        assert((read32(records[i]+0xa8)&~0x7ffu)==0xabc00000u);
    }
    assert(sum==n);
}
static void complete_match(unsigned n,unsigned mode) {
    c07_bowling_plan plan;
    fixture(n); write32(match_data+0x211f4,mode);
    if (mode) match_data[0x2120d]=0; /* Simulation must override human assignment. */
    assert(opening_hook(match_data)==37);
    assert(opening_calls==1 && end_calls==0 && cache[0].valid);
    plan=cache[0].plan;
    for (unsigned i=0;i<n;++i) {
        unsigned current=index_of(read_pointer(match_data+0xd540));
        assert(current==plan.bowler[i]);
        assert((read32(records[current]+0x8c)&63)==1);
        if (i) assert(current!=plan.bowler[i-1]);
        /* Fatigue, spell counters and rating changes cannot alter the plan. */
        memset(records[current]+0x80,0,4); match_data[0x58d6]=0;
        finish_over(i+1);
        assert(end_calls==0 && batter_swaps==i+1 && bowler_swaps==i+1);
    }
    assert(refreshes==(mode==2?n:0)); check_totals(n);
}
static void test_gates_and_roles(void) {
    fixture(20); match_data[0x2120d]=0; /* Human; stale CPU frontend flag irrelevant. */
    assignments[0][0]=1;
    assert(!cpu_bowling(match_data));
    assert(opening_hook(match_data)==37); assert(!cache[0].valid);
    end_over_hook(match_data); assert(end_calls==1 && batter_swaps==0);
    fixture(30); assert(opening_hook(match_data)==37); end_over_hook(match_data);
    assert(end_calls==1 && !cache[0].valid);
    fixture(65535); end_over_hook(match_data); assert(end_calls==1);
    fixture(20); put16(records[5],100); opening_hook(match_data);
    assert(!cache[0].valid); end_over_hook(match_data); assert(end_calls==1);
    fixture(20); set_bowling_role(records[2],4); opening_hook(match_data);
    assert(!cache[0].valid);
    fixture(5); write_pointer(match_data+0xd55c,NULL); /* B6 unnecessary in T5. */
    opening_hook(match_data); assert(cache[0].valid);
    assert((read32(records[4]+0x8c)&63)==0); /* Native opening role removed. */
    assert((read32(records[5]+0x8c)&63)==0);
    fixture(20); opening_hook(match_data);
    world[0x65]=0; finish_over(1); assert(world[0x65]==0); /* Existing B2 retained. */
    finish_over(2); assert(world[0x65]==0); /* Existing B1 retained. */
    finish_over(3); finish_over(4); assert(world[0x65]==1); /* B3 replaces B1. */
    fixture(20); opening_hook(match_data); world_present=0;
    for (unsigned i=1;i<=5;++i) finish_over(i);
    assert(end_calls==0 && !world[0x65]);
}
static void test_cpu_detection_regression(void) {
    for (unsigned side=0;side<2;++side) {
        for (unsigned absent=0;absent<3;++absent) {
            c07_bowling_plan plan;
            fixture(10); match_data[0x2120d]=0;
            write32(match_data+0xd568,side?13:12);
            /* Live failure: CPU slot0, no controller actor. Also verify one
               absent actor independently, matching the two native branches. */
            controller_values[0]=absent==1?1:0;
            controller_values[1]=absent==2?1:0;
            assert(cpu_bowling(match_data) && cpu_opening(match_data));
            assert(opening_hook(match_data)==37 && cache[0].valid);
            plan=cache[0].plan;
            for (unsigned over=0;over<10;++over) {
                assert(index_of(read_pointer(match_data+0xd540))==plan.bowler[over]);
                finish_over(over+1);
            }
            assert(end_calls==0); check_totals(10);
        }
        fixture(10); match_data[0x2120d]=0;
        write32(match_data+0xd568,side?13:12);
        assignments[side][0]=1;
        /* Opening actor state can still describe the CPU from last innings;
           fresh device assignments must protect the new human side. */
        controller_values[0]=controller_values[1]=0;
        assert(!cpu_opening(match_data));
        opening_hook(match_data); assert(!cache[0].valid);
        controller_values[0]=controller_values[1]=1;
        assert(!cpu_bowling(match_data)); end_over_hook(match_data); assert(end_calls==1);
        fixture(10); match_data[0x2120d]=0;
        write32(match_data+0xd568,side?13:12);
        assignments[side][1]=2; /* A second assigned device is also human. */
        assert(!cpu_opening(match_data));
    }
}
static void test_resume(void) {
    for (unsigned n=5;n<=20;n=n==5?10:20) {
        c07_bowling_plan old;
        unsigned checkpoint=n==20?11:4;
        fixture(n); opening_hook(match_data); old=cache[0].plan;
        for (unsigned i=0;i<checkpoint;++i) {
            BYTE *p=records[old.bowler[i]];
            write32(p+0xa8,read32(p+0xa8)+1);
        }
        put16(match_data+0x58c6,(uint16_t)checkpoint);
        write_pointer(match_data+0xd540,records[old.bowler[checkpoint-1]]);
        write_pointer(match_data+0xd544,records[old.bowler[checkpoint-2]]);
        memset(cache,0,sizeof(cache));
        end_over_hook(match_data);
        assert(!end_calls && cache[0].valid);
        for (unsigned i=checkpoint;i<n;++i) {
            assert(index_of(read_pointer(match_data+0xd540))==cache[0].plan.bowler[i]);
            finish_over(i+1);
        }
        check_totals(n);
        if (n==20) break;
    }
    fixture(10); opening_hook(match_data);
    write32(records[0]+0xa8,3); put16(match_data+0x58c6,3);
    end_over_hook(match_data); assert(end_calls==1 && !cache[0].valid);
    fixture(20); opening_hook(match_data); match_data[0x58c9]=1;
    opening_hook(match_data); assert(cache[0].valid && cache[1].valid);
}

static void field_clock(unsigned over) {
    put16(match_data+0x58c6,(uint16_t)over);
    put16(match_data+0x58e4,(uint16_t)over);
}
static void test_striker_run_storage(void) {
    fixture(20);
    assert(current_striker_runs(match_data)==0);
    write_pointer(match_data+0xd538,records[0]);
    for (unsigned runs=0;runs<=2047;++runs) {
        write32(records[0]+0xa4,0x800fffffu|(runs<<20));
        assert(current_striker_runs(match_data)==runs);
    }
    write32(records[1]+0xa4,10u<<20);
    write_pointer(match_data+0xd538,records[1]);
    assert(current_striker_runs(match_data)==10); /* Current striker, not team or non-striker. */
}
static void test_field_phases(void) {
    const unsigned formats[]={40,45,50};
    const int banks[]={0,35,140};
    for (unsigned f=0;f<3;++f) {
        unsigned n=formats[f],p=n/5;
        fixture(n);
        for (unsigned over=0;over<n;++over) {
            C07FieldPolicy policy;
            field_clock(over);
            assert(current_policy(&policy));
            assert(policy.outside_limit==(over<p?2:over<4*p?4:5));
            assert(policy.allowed==(over<p?C07_OPEN:over<2*p?C07_NORMAL:over<4*p?(C07_OPEN|C07_NORMAL):(C07_OPEN|C07_NORMAL|C07_DEFEND)));
            for (unsigned b=0;b<3;++b) for (unsigned family=0;family<3;++family) {
                native_field_id=banks[b]+(int)family*10+9;
                {
                    int selected=field_select_hook(),local=selected-banks[b];
                    unsigned selection=over<p?C07_OPEN:over<4*p?C07_NORMAL:C07_DEFEND;
                    assert(local>=0 && local<30 && local%10==1);
                    assert(selection&field_family(local));
                    if (selection&field_family((int)family*10)) assert(selected==banks[b]+(int)family*10+1);
                }
            }
            /* Model an XML/adaptive result with all nine fielders deep.
               Only excess outfielders move; keeper and bowler remain untouched. */
            field_prepare_hook(field_manager);
            assert(field_prepares==over+1);
            {
                C07Vec3 actual[9];
                for (unsigned i=0;i<9;++i) {
                    memcpy(&actual[i],field_actors[i],sizeof(actual[i]));
                    assert(actual[i].y==pending_positions[i].y);
                }
                assert(C07OutsideCount(actual,0x1ff)==policy.outside_limit);
                for (unsigned i=9;i<11;++i) assert(!memcmp(field_actors[i],&pending_positions[i],sizeof(C07Vec3)));
            }
        }
        /* Simulation uses the innings counter even with a stale global clock. */
        write32(match_data+0x211f4,2); put16(match_data+0x58c6,0);
        put16(match_data+0x58e4,(uint16_t)(2*p));
        {
            C07FieldPolicy policy;
            assert(current_policy(&policy) && policy.allowed==(C07_OPEN|C07_NORMAL));
        }
    }
    fixture(20); native_field_id=54;
    assert(field_select_hook()==56); field_prepare_hook(field_manager);
    assert(position_sets==0);
    fixture(50); field_clock(12);
    controller_object[5]=0; write_pointer(controller_object+0x44,world); write_pointer(world+4,ui);
    native_field_id=39; assert(field_select_hook()==39); /* Human field path ignores selector. */
    field_manager[5]=0; write_pointer(field_manager+0x44,world);
    field_prepare_hook(field_manager); assert(position_sets==0);
    fixture(50); native_field_id=99; assert(field_select_hook()==99);
    /* Native practice mode deliberately parks actors at a non-playing spot;
       even a retained long-format setting must not bring them back inside. */
    fixture(50); field_clock(12); practice_field_mode=1;
    for (unsigned i=0;i<11;++i) pending_positions[i]=(C07Vec3){45,0,45};
    field_prepare_hook(field_manager);
    assert(field_prepares==1 && position_sets==0);
    for (unsigned i=0;i<11;++i)
        assert(!memcmp(field_actors[i],&pending_positions[i],sizeof(C07Vec3)));
}
static void assert_field_choice(unsigned n,unsigned completed,unsigned runs,unsigned expected) {
    const int banks[]={0,35,140},variants[]={0,4,9};
    unsigned policy=0;
    assert(C07FieldSelectionFor(n,completed,runs,&policy) && policy==expected);
    field_clock(completed);
    write_pointer(match_data+0xd538,records[0]);
    write32(records[0]+0xa4,0x80054321u|(runs<<20));
    for (unsigned b=0;b<3;++b) for (unsigned input=0;input<3;++input) for (unsigned v=0;v<3;++v) {
        int selected,local;
        native_field_id=banks[b]+(int)input*10+variants[v];
        selected=field_select_hook(); local=selected-banks[b];
        assert(local>=0 && local<30 && local%10==1);
        assert(field_family(local)==expected);
    }
}
static void test_field_thresholds(void) {
    /* Independent boundary cases from the requested first defensive over and
       inclusive striker-run limit. Completed-over counters are zero based. */
    const struct { unsigned n,pp,first_defensive,limit; } specs[]={
        {10,3,7,20},{20,6,13,30}
    };
    for (unsigned i=0;i<2;++i) {
        unsigned n=specs[i].n,pp=specs[i].pp,cutoff=specs[i].first_defensive-1,limit=specs[i].limit;
        fixture(n);
        assert_field_choice(n,0,0,C07_OPEN);
        assert_field_choice(n,pp-1,2047,C07_OPEN); /* Powerplay overrides high runs. */
        assert_field_choice(n,pp,limit,C07_NORMAL); /* Equality is Normal. */
        assert_field_choice(n,pp,limit+1,C07_DEFEND);
        assert_field_choice(n,cutoff-1,0,C07_NORMAL);
        assert_field_choice(n,cutoff-1,limit,C07_NORMAL);
        assert_field_choice(n,cutoff-1,limit+1,C07_DEFEND);
        assert_field_choice(n,cutoff,0,C07_DEFEND); /* Format threshold alone. */
        assert_field_choice(n,cutoff,limit,C07_DEFEND);
        /* Switching strike recalculates from the new striker. */
        field_clock(pp); write32(records[1]+0xa4,0x80000000u|(limit<<20));
        write_pointer(match_data+0xd538,records[1]);
        native_field_id=39; assert(field_select_hook()==46);
        write32(records[1]+0xa4,0x80000000u|((limit+1)<<20));
        assert(field_select_hook()==36);
        /* Simulation must not use its stale global completed-over counter. */
        write32(match_data+0x211f4,2); put16(match_data+0x58c6,0);
        put16(match_data+0x58e4,(uint16_t)cutoff);
        write32(records[1]+0xa4,0);
        native_field_id=59; assert(field_select_hook()==36);
    }
    fixture(5);
    assert_field_choice(5,0,2047,C07_OPEN);
    for (unsigned over=1;over<5;++over) {
        assert_field_choice(5,over,0,C07_DEFEND);
        assert_field_choice(5,over,2047,C07_DEFEND);
    }
    {
        const struct { unsigned n,pp,first_defensive; } long_specs[]={
            {40,8,33},{45,9,37},{50,10,41}
        };
        const unsigned scores[]={0,10,20,40,41,100,2047};
        for (unsigned i=0;i<3;++i) {
            unsigned n=long_specs[i].n,pp=long_specs[i].pp,cutoff=long_specs[i].first_defensive-1;
            fixture(n);
            for (unsigned s=0;s<sizeof(scores)/sizeof(scores[0]);++s) {
                assert_field_choice(n,pp-1,scores[s],C07_OPEN);
                assert_field_choice(n,pp,scores[s],C07_NORMAL);
                assert_field_choice(n,cutoff-1,scores[s],C07_NORMAL);
                assert_field_choice(n,cutoff,scores[s],C07_DEFEND);
            }
            field_clock(pp); write_pointer(match_data+0xd538,records[1]);
            native_field_id=39;
            write32(records[1]+0xa4,0); assert(field_select_hook()==46);
            write32(records[1]+0xa4,2047u<<20); assert(field_select_hook()==46);
            /* Stale interactive clock must not keep simulated overs in PP. */
            write32(match_data+0x211f4,2); put16(match_data+0x58c6,0);
            put16(match_data+0x58e4,(uint16_t)cutoff);
            native_field_id=59; assert(field_select_hook()==36);
        }
    }
    {
        unsigned unchanged=123;
        assert(!C07FieldSelectionFor(30,10,50,&unchanged) && unchanged==123);
        assert(!C07FieldSelectionFor(10,4,5,NULL));
    }
}
static void test_field_legality(void) {
    BYTE reason;
    fixture(50); field_clock(10);
    for (unsigned i=0;i<4;++i) field_config[0x10+0x50*i]=(BYTE)i;
    assert(field_validate_hook(field_config,&reason)==1);
    field_config[0x10+0x50*4]=4;
    assert(field_validate_hook(field_config,&reason)==0 && reason==2);
    field_config[0x10+0x50*4]=0xff;
    assert(field_move_hook(field_config,4,&reason)==0 && reason==2); /* Empty deep candidate included. */
    field_config[0x10+0x50*4]=9; /* Selected player's old position excluded. */
    assert(field_move_hook(field_config,5,&reason)==0 && reason==2);
    field_config[0x10+0x50*3]=9; field_config[0x10+0x50*4]=0xff;
    assert(field_move_hook(field_config,5,&reason)==1); /* Three others plus moved fielder. */
    for (unsigned i=0;i<5;++i) field_config[0x10+0x50*i]=(BYTE)i;
    assert(field_move_hook(field_config,60,&reason)==1); /* Allows escaping restricted categories. */
    field_result=0; field_reason=4;
    assert(field_validate_hook(field_config,&reason)==0 && reason==4); /* Native failures preserved. */
    field_result=1; field_reason=1;
    memset(field_config+0x282d,50,9);
    for (unsigned i=0;i<4;++i) field_config[0x282d+i]=(BYTE)i;
    assert(field_preset_hook(field_config,0,1,0)==1);
    field_config[0x282d+4]=4;
    assert(field_preset_hook(field_config,0,1,0)==0);
    assert(field_preset_hook(field_config,0,0,0)==1); /* Explicit unrestricted preview. */
    assert(strstr(field_text_hook(world,0x20e),"four")!=NULL);
    assert(!strcmp(field_text_hook(world,0x20d),"native text"));
    field_clock(40);
    assert(field_validate_hook(field_config,&reason)==1);
    assert(field_preset_hook(field_config,0,1,0)==1);
    assert(!strcmp(field_text_hook(world,0x20e),"native text"));
    fixture(20); assert(!strcmp(field_text_hook(world,0x20e),"native text"));
}
static void test_field_variants(void) {
    const unsigned formats[]={5,10,20,40,45,50};
    const int banks[]={0,35,140};
    for (unsigned f=0;f<6;++f) {
        unsigned previous,next;
        uint64_t saved_rng;
        fixture(formats[f]);
        native_field_id=59;
        assert(field_select_hook()==56); /* Open2 regardless of native variant. */
        saved_rng=variant_random_state;
        for (unsigned b=0;b<3;++b) for (unsigned family=0;family<3;++family)
            for (unsigned call=0;call<100;++call) {
                native_field_id=banks[b]+(int)family*10+(int)(call%10);
                assert(field_select_hook()==banks[b]+21);
            }
        assert(variant_random_state==saved_rng);
        /* Four runs without a credited boundary, odd/even totals, strike and
           over/bowler changes do not trigger a new variant. */
        write32(batting_records[0][0]+0xa4,4u<<20);
        write_pointer(match_data+0xd538,batting_records[0][0]);
        put16(match_data+0x5878,21);
        native_field_id=0; assert(field_select_hook()==21);
        write_pointer(match_data+0xd538,batting_records[0][1]);
        field_clock((unsigned)powerplay_hook((int)formats[f]));
        native_field_id=149;
        assert(field_select_hook()==(formats[f]==5?141:151));
        assert(variant_random_state==saved_rng);
        /* Credited four on the previous striker survives a change of strike. */
        write32(batting_records[0][0]+0xa4,(4u<<20)|(1u<<10));
        next=(unsigned)(field_select_hook()-140)%10;
        assert(next<5 && next!=1); previous=next;
        saved_rng=variant_random_state;
        for (unsigned i=0;i<1000;++i) assert((unsigned)(field_select_hook()-140)%10==previous);
        assert(variant_random_state==saved_rng);
        /* Six, wicket/run-out and simultaneous event counters each change once. */
        write32(batting_records[0][0]+0xa4,(10u<<20)|(1u<<10)|1u);
        next=(unsigned)(field_select_hook()-140)%10;
        assert(next<5 && next!=previous); previous=next;
        put16(match_data+0x5910,1);
        write_pointer(match_data+0xd538,batting_records[0][2]);
        next=(unsigned)(field_select_hook()-140)%10;
        assert(next<5 && next!=previous); previous=next;
        write32(batting_records[0][1]+0xa4,1u<<10); put16(match_data+0x5910,2);
        next=(unsigned)(field_select_hook()-140)%10;
        assert(next<5 && next!=previous); previous=next;
        saved_rng=variant_random_state;
        for (unsigned i=0;i<100;++i) assert((unsigned)(field_select_hook()-140)%10==previous);
        assert(variant_random_state==saved_rng);
        /* Separate innings state; returning to a cached innings is stable. */
        match_data[0x58c9]=1; write_pointer(match_data+0xd530,batting_records[1][0]);
        field_clock(0); assert(field_select_hook()==161);
        put16(match_data+0x5912,1);
        assert((unsigned)(field_select_hook()-160)!=1);
        match_data[0x58c9]=0; write_pointer(match_data+0xd530,batting_records[0][0]);
        assert((unsigned)(field_select_hook()-160)==previous);
        /* Explicit opening resets a reused match address, without replaying
           any already-present boundary/wicket history. */
        opening_hook(match_data); assert(field_select_hook()==161);
    }
    fixture(20);
    {
        uint64_t scheduling_rng=random_state;
        for (unsigned current=0;current<5;++current) {
            unsigned seen=0;
            for (unsigned i=0;i<4096;++i) {
                unsigned choice=next_field_variant(current);
                assert(choice<5 && choice!=current); seen|=1u<<choice;
            }
            assert(seen==(31u^(1u<<current)));
        }
        assert(random_state==scheduling_rng); /* Rotation RNG remains untouched. */
    }
    /* 10,000 credited boundaries: no out-of-range or consecutive repeats. */
    fixture(50); native_field_id=59;
    {
        unsigned previous=(unsigned)(field_select_hook()-55);
        for (unsigned event=0;event<10000;++event) {
            unsigned next;
            write32(batting_records[0][event/1000]+0xa4,event%1000+1);
            next=(unsigned)(field_select_hook()-55);
            assert(next<5 && next!=previous); previous=next;
        }
    }
    /* Resume: baseline history, then react only to a new event, in simulation. */
    fixture(10); native_field_id=59; write32(match_data+0x211f4,2);
    write32(batting_records[0][0]+0xa4,(7u<<10)|3u); put16(match_data+0x5910,4);
    assert(field_select_hook()==56);
    write32(batting_records[0][0]+0xa4,(8u<<10)|3u);
    assert(field_select_hook()!=56);
    /* Human and unsupported-format calls must not advance the variant state. */
    fixture(30); native_field_id=59; assert(field_select_hook()==59);
    assert(!field_variants[0].valid);
    fixture(20); controller_object[5]=0;
    write_pointer(controller_object+0x44,world); write_pointer(world+4,ui);
    native_field_id=59; assert(field_select_hook()==59); assert(!field_variants[0].valid);
    /* A late or rebuilt record pointer starts a new baseline safely. */
    fixture(20); write_pointer(match_data+0xd530,NULL); native_field_id=59;
    assert(field_select_hook()==56 && !field_variants[0].valid);
    write_pointer(match_data+0xd530,batting_records[0][0]);
    assert(field_select_hook()==56 && field_variants[0].valid);
    write32(batting_records[0][0]+0xa4,1); assert(field_select_hook()!=56);
    write32(batting_records[0][0]+0xa4,0); assert(field_select_hook()==56);
}
static void test_physical_circle(void) {
    C07Vec3 p[9]={{0}};
    unsigned mask;
    p[0]=(C07Vec3){27.0f,2,0};
    p[1]=(C07Vec3){0,3,37.0f}; /* Inside the far wicket's end arc. */
    p[2]=(C07Vec3){0,4,-37.0f};
    p[3]=(C07Vec3){28,5,0};
    p[4]=(C07Vec3){0,6,38};
    p[5]=(C07Vec3){0,7,-50};
    assert(C07OutsideCount(p,0x1ff)==3);
    assert(C07LimitOutside(p,0x1ff,2,&mask)==1 && mask==(1u<<4)); /* Closest to oval boundary. */
    assert(C07OutsideCount(p,0x1ff)==2 && p[4].y==6);
    assert(C07LimitOutside(p,0x1ff,2,&mask)==0 && mask==0);
    assert(C07OutsideCount(p,7)==0);
}

static unsigned detour_calls;
static int __cdecl replacement_open(BYTE *p) { (void)p; ++detour_calls; return 77; }
static void __cdecl replacement_end(BYTE *p) { (void)p; ++detour_calls; }
static int __stdcall replacement_pp(int n) { ++detour_calls; return n*2; }
static int __cdecl replacement_select(void) { ++detour_calls; return 77; }
static BYTE THISCALL replacement_move(BYTE *c,int p,BYTE *r) { assert(c==field_config && p==7 && r==world); ++detour_calls; return 77; }
static BYTE THISCALL replacement_validate(BYTE *c,BYTE *r) { assert(c==field_config && r==world); ++detour_calls; return 77; }
static BYTE THISCALL replacement_preset(BYTE *c,int p,int l,int pp) { assert(c==field_config && p==7 && l==1 && pp==0); ++detour_calls; return 77; }
static const char *THISCALL replacement_text(BYTE *c,unsigned id) { assert(c==field_config && id==0x20e); ++detour_calls; return "replacement"; }
static void THISCALL replacement_prepare(BYTE *c) { assert(c==field_manager); ++detour_calls; }
static void test_detours(void) {
    BYTE *image=VirtualAlloc(NULL,0x400000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    C07Hook h[C07_PLAN_HOOKS];
    MEMORY_BASIC_INFORMATION protection;
    DWORD old_protection;
    const BYTE open_end[]={0xb8,0x7b,0,0,0,0x5e,0x5b,0xc3};
    const BYTE end_end[]={0x5e,0xc3}, pp_end[]={0xc2,4,0};
    const BYTE selector_end[]={0xb8,0x7b,0,0,0,0x5f,0x5e,0x5d,0x5b,0x59,0xc3};
    const BYTE move_end[]={0xb8,1,0,0,0,0x5b,0x83,0xc4,0x10,0xc2,8,0};
    const BYTE validate_end[]={0xb8,1,0,0,0,0x5e,0x5d,0x5b,0x59,0xc2,4,0};
    const BYTE preset_end[]={0xb8,1,0,0,0,0x83,0xc4,8,0xc2,12,0};
    const BYTE text_end[]={0xb8,0x7b,0,0,0,0x89,0xec,0x5d,0xc2,4,0};
    const BYTE prepare_end[]={0x81,0xc4,0x38,2,0,0,0xc3};
    assert(image);
    memcpy(h,hooks,sizeof(h));
    h[0].replacement=(void *)&replacement_open; h[1].replacement=(void *)&replacement_end;
    h[2].replacement=(void *)&replacement_pp;
    h[3].replacement=(void *)&replacement_select; h[4].replacement=(void *)&replacement_move;
    h[5].replacement=(void *)&replacement_validate; h[6].replacement=(void *)&replacement_preset;
    h[7].replacement=(void *)&replacement_text; h[8].replacement=(void *)&replacement_prepare;
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) {
        h[i].trampoline=NULL; memcpy(image+h[i].rva,h[i].expected,h[i].length);
    }
    memcpy(image+h[0].rva+h[0].length,open_end,sizeof(open_end));
    memcpy(image+h[1].rva+h[1].length,end_end,sizeof(end_end));
    memcpy(image+h[2].rva+h[2].length,pp_end,sizeof(pp_end));
    memcpy(image+h[3].rva+h[3].length,selector_end,sizeof(selector_end));
    memcpy(image+h[4].rva+h[4].length,move_end,sizeof(move_end));
    memcpy(image+h[5].rva+h[5].length,validate_end,sizeof(validate_end));
    memcpy(image+h[6].rva+h[6].length,preset_end,sizeof(preset_end));
    memcpy(image+h[7].rva+h[7].length,text_end,sizeof(text_end));
    memcpy(image+h[8].rva+h[8].length,prepare_end,sizeof(prepare_end));
    assert(VirtualProtect(image,0x400000,PAGE_EXECUTE_READ,&old_protection));
    assert(FlushInstructionCache(GetCurrentProcess(),image,0x400000));
    h[8].expected[0]^=1;
    assert(!C07InstallPlanHooks(image,h));
    assert(!memcmp(image+h[0].rva,h[0].expected,h[0].length));
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) assert(!h[i].trampoline);
    assert(VirtualQuery(image+h[4].rva,&protection,sizeof(protection))==sizeof(protection));
    assert(protection.Protect==PAGE_EXECUTE_READ);
    h[8].expected[0]^=1;
    assert(C07InstallPlanHooks(image,h));
    /* Move/full-field/preset validators share one page. Later snapshots see
       RWX, so only reverse restoration leaves the original RX protection. */
    assert((h[4].rva&~0xfffu)==(h[5].rva&~0xfffu));
    assert((h[4].rva&~0xfffu)==(h[6].rva&~0xfffu));
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) {
        assert(VirtualQuery(image+h[i].rva,&protection,sizeof(protection))==sizeof(protection));
        assert(protection.Protect==PAGE_EXECUTE_READ);
    }
    for (unsigned i=0;i<10000;++i) {
        assert(((OpeningFn)(image+h[0].rva))(NULL)==77);
        ((MatchFn)(image+h[1].rva))(NULL);
        assert(((PowerplayFn)(image+h[2].rva))(20)==40);
        assert(((OpeningFn)h[0].trampoline)(NULL)==123);
        ((MatchFn)h[1].trampoline)(NULL);
        assert(((PowerplayFn)h[2].trampoline)(20)==15);
        assert(((FieldSelectFn)(image+h[3].rva))()==77);
        assert(((FieldSelectFn)h[3].trampoline)()==123);
        assert(((MoveFieldFn)(image+h[4].rva))(field_config,7,world)==77);
        assert(((MoveFieldFn)h[4].trampoline)(field_config,7,world)==1);
        assert(((ValidateFieldFn)(image+h[5].rva))(field_config,world)==77);
        assert(((ValidateFieldFn)h[5].trampoline)(field_config,world)==1);
        assert(((PresetFieldFn)(image+h[6].rva))(field_config,7,1,0)==77);
        assert(((PresetFieldFn)h[6].trampoline)(field_config,7,1,0)==1);
        assert(!strcmp(((TextFn)(image+h[7].rva))(field_config,0x20e),"replacement"));
        assert((uintptr_t)((TextFn)h[7].trampoline)(field_config,0x20e)==123);
        ((PrepareFieldFn)(image+h[8].rva))(field_manager);
        ((PrepareFieldFn)h[8].trampoline)(field_manager);
    }
    assert(detour_calls==90000);
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) VirtualFree(h[i].trampoline,0,MEM_RELEASE);
    VirtualFree(image,0,MEM_RELEASE);
}
static void test_executable(const char *path) {
    FILE *f=fopen(path,"rb");
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS32 pe;
    IMAGE_SECTION_HEADER sections[32];
    assert(f && fread(&dos,sizeof(dos),1,f)==1 && dos.e_magic==IMAGE_DOS_SIGNATURE);
    assert(!fseek(f,dos.e_lfanew,SEEK_SET) && fread(&pe,sizeof(pe),1,f)==1);
    assert(pe.FileHeader.NumberOfSections<=32 && pe.OptionalHeader.SizeOfImage==6606446u);
    assert(fread(sections,sizeof(sections[0]),pe.FileHeader.NumberOfSections,f)==pe.FileHeader.NumberOfSections);
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) {
        unsigned found=0;
        for (unsigned s=0;s<pe.FileHeader.NumberOfSections;++s) {
            DWORD rva=hooks[i].rva, base=sections[s].VirtualAddress;
            if (rva>=base && rva+hooks[i].length<=base+sections[s].SizeOfRawData) {
                BYTE actual[8];
                assert(!fseek(f,(long)(sections[s].PointerToRawData+rva-base),SEEK_SET));
                assert(fread(actual,hooks[i].length,1,f)==1);
                assert(!memcmp(actual,hooks[i].expected,hooks[i].length)); ++found;
            }
        }
        assert(found==1);
    }
    fclose(f);
}
static void test_dll(const char *path) {
    HMODULE dll=LoadLibraryA(path);
    typedef void (__cdecl *InitFn)(void);
    typedef LONG (__cdecl *StatusFn)(void);
    InitFn init;
    StatusFn state;
    assert(dll);
    init=(InitFn)(uintptr_t)GetProcAddress(dll,"InitializeASI");
    state=(StatusFn)(uintptr_t)GetProcAddress(dll,"GetBowlingPlanStatus");
    assert(init && state && state()==0);
    init(); assert(state()==-1); init(); assert(state()==-1);
    assert(FreeLibrary(dll));
}
static void test_hash_and_initialization(const char *exe) {
    WCHAR source[MAX_PATH],temp_dir[MAX_PATH],copy[MAX_PATH];
    char original_digest[65],modified_digest[65],host_digest[65],copy_narrow[MAX_PATH];
    DWORD written;
    HANDLE file;
    BYTE *image;
    C07Hook saved_hooks[C07_PLAN_HOOKS];
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS32 *pe;
    assert(MultiByteToWideChar(CP_ACP,0,exe,-1,source,MAX_PATH));
    assert(executable_hash(source,original_digest)>=0 && strlen(original_digest)==64);
    assert(GetTempPathW(MAX_PATH,temp_dir)>0 && GetTempFileNameW(temp_dir,L"c07",0,copy));
    assert(CopyFileW(source,copy,FALSE));
    file=CreateFileW(copy,FILE_APPEND_DATA,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);
    assert(WriteFile(file,"C07 harmless test footer",24,&written,NULL) && written==24);
    assert(CloseHandle(file));
    assert(executable_hash(copy,modified_digest)==0);
    assert(strcmp(original_digest,modified_digest));
    assert(WideCharToMultiByte(CP_ACP,0,copy,-1,copy_narrow,MAX_PATH,NULL,NULL));
    test_executable(copy_narrow); /* Added footer preserves layout and every hook. */
    assert(DeleteFileW(copy));
    assert(executable_hash(copy,modified_digest)==-1 && !modified_digest[0]);
    /* Initialize under the deliberately nonmatching test-host executable.
       A compatible mapped image must still receive all nine hooks. */
    assert(GetModuleFileNameW(NULL,source,MAX_PATH));
    assert(executable_hash(source,host_digest)==0);
    image=VirtualAlloc(NULL,6606446,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(image);
    dos=(IMAGE_DOS_HEADER *)image; dos->e_magic=IMAGE_DOS_SIGNATURE; dos->e_lfanew=0x80;
    pe=(IMAGE_NT_HEADERS32 *)(image+dos->e_lfanew);
    pe->Signature=IMAGE_NT_SIGNATURE; pe->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    pe->OptionalHeader.SizeOfImage=6606446;
    memcpy(saved_hooks,hooks,sizeof(hooks));
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) {
        hooks[i].trampoline=NULL;
        memcpy(image+hooks[i].rva,hooks[i].expected,hooks[i].length);
    }
    pe->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;
    status=0; initialize_image(image); assert(status==-1);
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) assert(!hooks[i].trampoline);
    pe->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    image[hooks[0].rva]^=1;
    status=0; initialize_image(image); assert(status==-2);
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) assert(!hooks[i].trampoline);
    image[hooks[0].rva]^=1;
    status=0; initialize_image(image); assert(status==1);
    for (unsigned i=0;i<C07_PLAN_HOOKS;++i) {
        assert(hooks[i].trampoline && image[hooks[i].rva]==0xe9);
        assert(VirtualFree(hooks[i].trampoline,0,MEM_RELEASE));
    }
    memcpy(hooks,saved_hooks,sizeof(hooks));
    assert(VirtualFree(image,0,MEM_RELEASE));
}
int main(int argc,char **argv) {
    assert(argc==3);
    for (unsigned mode=0;mode<=2;++mode) {
        complete_match(5,mode); complete_match(10,mode); complete_match(20,mode);
        complete_match(40,mode); complete_match(45,mode); complete_match(50,mode);
    }
    test_gates_and_roles(); test_cpu_detection_regression(); test_resume();
    test_striker_run_storage(); test_field_phases(); test_field_thresholds(); test_field_legality(); test_field_variants(); test_physical_circle();
    for (unsigned i=0;i<10000;++i) {
        assert(powerplay_hook(5)==1 && powerplay_hook(10)==3 && powerplay_hook(20)==6);
        assert(powerplay_hook(40)==8 && powerplay_hook(45)==9 && powerplay_hook(50)==10);
        assert(powerplay_hook(-1)==99 && powerplay_hook(15)==115 && powerplay_hook(30)==130);
    }
    test_detours(); test_executable(argv[2]); test_dll(argv[1]); test_hash_and_initialization(argv[2]);
    puts("Native integration: 6 formats, CPU/human gates, field families/2-4-5 limits, editor/presets/text, adaptive placement, 9 x86 detours, real EXE prefixes PASS.");
    return 0;
}
