// Standalone unit test for uc_foldCompletedWrite() (pipeline/ContiguousWatermark.h) -- the
// contiguous-from-0 low-water mark that makes media-reconnect RESUME safe on the pipelined
// (io_uring / IOCP) backends. The backends complete writes OUT OF ORDER, so the only offset
// safe to resume from is the largest N with every byte in [0,N) written. This test drives the
// exact sequences the adversarial review called out: out-of-order completion above a hole,
// short-write partial+remainder, and multi-extent absorption.
//
// Built + run by cases/watermark_unit.py. Returns 0 iff every check passes.
#include "../../pipeline/ContiguousWatermark.h"
#include <cstdio>
#include <vector>
#include <utility>
#include <cstdint>

static int g_fail = 0;
#define CHECK(cond, msg) do{ if(!(cond)){ std::printf("FAIL: %s (line %d)\n", msg, __LINE__); g_fail++; } }while(0)

typedef std::vector<std::pair<uint64_t,uint64_t> > Extents;

int main()
{
    // 1. In-order completions: contiguous advances by each, no pending buffered.
    {
        uint64_t c=0; Extents p;
        uc_foldCompletedWrite(c,p,0,100);   CHECK(c==100,"inorder a"); CHECK(p.empty(),"inorder a pend");
        uc_foldCompletedWrite(c,p,100,100);  CHECK(c==200,"inorder b"); CHECK(p.empty(),"inorder b pend");
        uc_foldCompletedWrite(c,p,200,50);   CHECK(c==250,"inorder c");
    }

    // 2. OUT-OF-ORDER: a high-offset write completes BEFORE the lower one. contiguous must stay at
    //    the low-water mark (NOT jump to the over-count), then leap once the hole is filled.
    {
        uint64_t c=0; Extents p;
        uc_foldCompletedWrite(c,p,64,64);    // [64,128) completes first -> buffered, hole below
        CHECK(c==0,"oo: contiguous must NOT advance past the hole");
        CHECK(p.size()==1,"oo: one pending extent");
        uc_foldCompletedWrite(c,p,0,64);     // [0,64) fills the hole -> absorbs [64,128) too
        CHECK(c==128,"oo: leaps to 128 after hole filled");
        CHECK(p.empty(),"oo: pending drained");
    }

    // 3. MULTI-EXTENT absorption: several out-of-order extents buffered, then one low write
    //    connects them all in a single fold.
    {
        uint64_t c=0; Extents p;
        uc_foldCompletedWrite(c,p,300,100);  // [300,400)
        uc_foldCompletedWrite(c,p,100,100);  // [100,200)
        uc_foldCompletedWrite(c,p,200,100);  // [200,300)
        CHECK(c==0,"multi: still 0 (nothing from offset 0 yet)");
        CHECK(p.size()==3,"multi: three pending");
        uc_foldCompletedWrite(c,p,0,100);    // [0,100) -> chains 0..100..200..300..400
        CHECK(c==400,"multi: chains all the way to 400");
        CHECK(p.empty(),"multi: all drained");
    }

    // 4. SHORT WRITE: a chunk [0,100) completes as a partial [0,40) then the remainder [40,100).
    //    Recorded per-completion with the live offset/len -> contiguous ends at 100 exactly.
    {
        uint64_t c=0; Extents p;
        uc_foldCompletedWrite(c,p,0,40);     // partial
        CHECK(c==40,"short: partial advances to 40");
        uc_foldCompletedWrite(c,p,40,60);    // remainder at advanced offset
        CHECK(c==100,"short: remainder advances to 100");
        CHECK(p.empty(),"short: nothing pending");
    }

    // 5. Short write of an OUT-OF-ORDER chunk: [200,300) arrives as [200,250)+[250,300) while
    //    [100,200) is still pending; all connect once [0,100)+[100,200) land.
    {
        uint64_t c=0; Extents p;
        uc_foldCompletedWrite(c,p,200,50);   // [200,250) buffered
        uc_foldCompletedWrite(c,p,250,50);   // [250,300) buffered (does NOT connect to 250? it does to the [200,250) only via the low-water path; here it buffers)
        uc_foldCompletedWrite(c,p,100,100);  // [100,200) buffered (still no 0)
        uc_foldCompletedWrite(c,p,0,100);    // [0,100) -> 0..100..200..250..300
        CHECK(c==300,"short-oo: chains to 300");
        CHECK(p.empty(),"short-oo: drained");
    }

    // 6. len==0 is a no-op (must never corrupt the mark or buffer an empty extent).
    {
        uint64_t c=128; Extents p;
        uc_foldCompletedWrite(c,p,128,0);
        CHECK(c==128,"zero-len no-op"); CHECK(p.empty(),"zero-len no pending");
    }

    // 7. A pseudo-random permutation of 16 contiguous 4 KiB chunks completing in scrambled order
    //    must end with contiguous == total and nothing left pending (the real backend invariant).
    {
        const uint64_t CH=4096; const int N=16;
        // a fixed scramble (no RNG dependency): reverse-ish interleave
        const int order[N]={7,3,11,1,15,9,5,13,0,8,4,12,2,10,6,14};
        uint64_t c=0; Extents p;
        for(int i=0;i<N;i++)
            uc_foldCompletedWrite(c,p,(uint64_t)order[i]*CH,CH);
        CHECK(c==(uint64_t)N*CH,"perm: contiguous == total");
        CHECK(p.empty(),"perm: nothing pending at the end");
    }

    if(g_fail==0)
        std::printf("watermark_test: ALL PASS\n");
    else
        std::printf("watermark_test: %d FAILURE(S)\n", g_fail);
    return g_fail==0?0:1;
}
