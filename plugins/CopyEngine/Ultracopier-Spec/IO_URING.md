# io_uring in Ultracopier

## What is io_uring?

io_uring is a Linux kernel interface (5.1+) for asynchronous I/O. It replaces older
async I/O mechanisms (POSIX AIO, libaio) with a design built around two lock-free
ring buffers shared between userspace and kernel:

```
Userspace                          Kernel
   |                                 |
   |  Submission Queue (SQ)          |
   |  [sqe][sqe][sqe]  --------->   |  executes operations
   |                                 |
   |  Completion Queue (CQ)          |
   |  <---------  [cqe][cqe][cqe]   |  posts results
```

- **SQE** (Submission Queue Entry): describes one operation (read, write, open, close,
  fsync, ...). The caller fills it in, then calls `io_uring_submit()`.
- **CQE** (Completion Queue Entry): the kernel writes the result here. The caller
  retrieves it with `io_uring_wait_cqe()` (blocking) or `io_uring_peek_cqe()`.
- **`cqe->res`**: the return value that the equivalent syscall would have returned.
  Negative values are negated errno codes (e.g. `-ENOENT`).

Key advantage: multiple operations can be submitted in a single syscall, and
completions can be batched too. There is no per-operation syscall overhead.

## Ring lifecycle

A ring is created once and reused for all I/O on that thread:

```
io_uring_queue_init(depth, &ring, flags)   -- allocate SQ/CQ with `depth` slots
... use ring ...
io_uring_queue_exit(&ring)                 -- free kernel resources
```

In this codebase the ring is created in `TransferThreadUring::run()` (the QThread
worker entry point) with `RING_DEPTH = 64` slots, and destroyed when the thread's
event loop exits. One ring per transfer thread; it is reused across all file
transfers handled by that thread.

## How a single I/O operation works (submit-then-wait pattern)

For operations where we need the result before continuing (open, close, fsync),
the pattern is always:

```cpp
// 1. Get a free SQE slot
struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

// 2. Prepare the operation
io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);

// 3. Tag it so we can identify it in CQE (optional for single-op submits)
io_uring_sqe_set_data64(sqe, OP_OPEN_TAG);

// 4. Submit to kernel
io_uring_submit(&ring);

// 5. Wait for completion
struct io_uring_cqe *cqe;
io_uring_wait_cqe(&ring, &cqe);

// 6. Read result (fd on success, -errno on failure)
int fd = cqe->res;

// 7. Tell kernel we consumed this CQE
io_uring_cqe_seen(&ring, cqe);
```

This is used for `openSourceFile()`, `openDestFile()`, and the fsync at the end
of a transfer. Even though each waits for its result, the operation goes through
the io_uring kernel path which can be more efficient than a direct syscall
(especially on newer kernels with SQPOLL or when combined with other in-flight
operations on the same ring).

## Pipelined (overlapped) data transfer

The main performance benefit comes from the copy pipeline in `doTransferPipeline()`,
where reads and writes are overlapped:

```
Time -->

Buffer 0:  [==READ==]                [==READ==]
Buffer 1:       [==READ==]                [==READ==]
Buffer 2:            [==READ==]
Buffer 3:  (waiting)      [==WRITE=]      [==WRITE=]
Buffer 0:                      [==WRITE=]
Buffer 1:                           [==WRITE=]
```

There are `NUM_BUFFERS = 8` pipeline buffers. The flow:

1. **Initial read burst**: submit reads into all free buffers (up to `NUM_BUFFERS`)
   in one `io_uring_submit()` call.
2. **Event loop**: call `io_uring_wait_cqe()` for at least one completion, then
   `io_uring_peek_batch_cqe()` to drain all available CQEs in one go.
3. **Read completion** -> queue a write for the same buffer (state: Reading -> Writing).
4. **Write completion** -> mark buffer Free, queue a new read if data remains.
5. **Single submit per iteration**: all SQEs queued during one batch of CQE processing
   are submitted together.
6. All processed CQEs are marked seen via `io_uring_cq_advance(&ring, numCqes)` in
   bulk instead of one-by-one `io_uring_cqe_seen()`.

### CQE tagging

Since reads and writes are in flight simultaneously, each SQE is tagged via
`io_uring_sqe_set_data64()` with a 64-bit value:

```
  bits 63..60: operation type     bits 59..0: buffer index
  |------------|------------------------------------------------|
  | OP_xxx_TAG |                 buffer index                   |
```

Constants:
| Tag            | Value                  | Operation      |
|----------------|------------------------|----------------|
| `OP_READ_TAG`  | `0x1000000000000000`   | pread          |
| `OP_WRITE_TAG` | `0x2000000000000000`   | pwrite         |
| `OP_FSYNC_TAG` | `0x3000000000000000`   | fsync          |
| `OP_OPEN_TAG`  | `0x4000000000000000`   | openat         |
| `OP_CLOSE_TAG` | `0x5000000000000000`   | close          |
| `OP_MASK`      | `0xF000000000000000`   | extract op     |
| `IDX_MASK`     | `0x0FFFFFFFFFFFFFFF`   | extract index  |

On CQE retrieval: `userData & OP_MASK` gives the operation type,
`userData & IDX_MASK` gives the buffer or fd index.

## Operations using io_uring

### open (openSourceFile / openDestFile)

Uses `io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode)`.

- Source: `O_RDONLY`
- Destination: `O_WRONLY | O_CREAT`, mode `0755`

`cqe->res` is the new file descriptor on success, or a negated errno on failure
(e.g. `-EACCES`). Error handling uses `strerror(-fd)` since the kernel returns
negated errno, not setting the thread-local `errno`.

After the fd is obtained, non-io_uring syscalls (`posix_fadvise`, `fstat`,
`ftruncate`, `lseek`) are used for metadata operations that have no significant
latency benefit from being async.

### read / write (doTransferPipeline)

Uses `io_uring_prep_read(sqe, fd, buf, len, offset)` and
`io_uring_prep_write(sqe, fd, buf, len, offset)`.

Reads and writes use explicit offsets (`pread`/`pwrite` semantics) so file position
is tracked in userspace (`readOffset`, `writeOffset`) and there is no dependency
on the kernel file position.

Short writes are handled: if `cqe->res < bytesUsed`, the remaining bytes are
shifted to the front of the buffer with `memmove` and a follow-up write is queued
for the remainder.

### fsync (end of transfer)

Uses `io_uring_prep_fsync(sqe, destFd, 0)` after all writes complete to ensure
data durability before closing.

### close (closeFiles)

Uses `io_uring_prep_close(sqe, fd)`. Both source and destination fds are submitted
as separate SQEs in a single `io_uring_submit()` call, then all CQEs are drained.

The close index (0 = source, 1 = dest) is encoded in the low bits of the tag
(`OP_CLOSE_TAG | index`) for error reporting.

Fallback: if the ring is not initialized (early destruction) or `io_uring_get_sqe`
fails (ring full), a blocking `::close()` is used instead.

## Error conventions

io_uring returns errors differently from POSIX syscalls:

| Context          | POSIX                    | io_uring                     |
|------------------|--------------------------|------------------------------|
| open failure     | returns -1, sets `errno` | `cqe->res` is `-errno`       |
| read failure     | returns -1, sets `errno` | `cqe->res` is `-errno`       |
| wait_cqe failure | n/a                      | returns `-errno` (ring error) |

The code always checks `cqe->res < 0` and uses `strerror(-cqe->res)` to get the
error message. For `io_uring_wait_cqe` itself, the return value is also a negated
errno (ring-level failure, e.g. `-EINTR`).

## Build

Enabled when `ULTRACOPIER_PLUGIN_IO_URING` is defined. Requires:
- Linux kernel >= 5.6 (for openat/close support in io_uring)
- `liburing` development headers and library (`liburing-dev` / `liburing-devel`)
- Link with `-luring`
