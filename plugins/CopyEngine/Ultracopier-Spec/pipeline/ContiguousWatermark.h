/** \file ContiguousWatermark.h
\brief Contiguous-from-0 low-water mark for out-of-order pipelined writes (resume support)
\author alpha_one_x86
\licence GPL3, see the file COPYING

The pipelined backends (io_uring / IOCP) keep several writes in flight and their completions
arrive OUT OF ORDER (a write at a higher offset can complete before a still-pending lower one).
The only byte offset safe to RESUME a media-reconnect transfer from is the largest N such that
EVERY byte in [0,N) is written -- NOT the total bytes written (which may sit above a hole).

uc_foldCompletedWrite() maintains that low-water mark: it advances `contiguous` only across
extents contiguous from 0 and buffers out-of-order extents in `pending` (bounded by the in-flight
buffer pool) until they connect. It is a free function so it can be unit-tested in isolation
(test/unit/watermark_test.cpp) AND used verbatim by TransferThreadPipelined::recordCompletedWrite. */

#ifndef ULTRACOPIER_CONTIGUOUSWATERMARK_H
#define ULTRACOPIER_CONTIGUOUSWATERMARK_H

#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>

/** Fold a just-completed destination write extent [off, off+len) into the contiguous low-water
 * mark `contiguous`. MUST be called once per write completion with the LIVE (post-short-write)
 * offset+length, never the logical chunk size. Out-of-order extents accumulate in `pending`. */
static inline void uc_foldCompletedWrite(uint64_t &contiguous,
                                         std::vector<std::pair<uint64_t,uint64_t> > &pending,
                                         uint64_t off, uint64_t len)
{
    if(len==0)
        return;
    if(off==contiguous)
    {
        // Extends the contiguous prefix; absorb any buffered extents that now connect to it.
        contiguous+=len;
        bool merged=true;
        while(merged)
        {
            merged=false;
            for(size_t i=0;i<pending.size();i++)
            {
                if(pending.at(i).first==contiguous)
                {
                    contiguous=pending.at(i).second;
                    pending.erase(pending.cbegin()+i);
                    merged=true;
                    break;
                }
            }
        }
    }
    else
    {
        // Out-of-order completion above a still-pending lower write: buffer until it connects.
        pending.push_back(std::pair<uint64_t,uint64_t>(off,off+len));
    }
}

#endif // ULTRACOPIER_CONTIGUOUSWATERMARK_H
