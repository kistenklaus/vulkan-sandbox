// SPDX-License-Identifier: Apache-2.0 OR MIT OR Unlicense

// A prefix sum.
//
// This test builds in three configurations. The default is a
// compatibility mode, essentially plain GLSL. With ATOMIC set, the
// flag loads and stores are atomic operations, but uses barriers.
// With both ATOMIC and VKMM set, it uses acquire/release semantics
// instead of barriers.

#version 450

#extension GL_KHR_memory_scope_semantics : enable

#ifdef VKMM
#pragma use_vulkan_memory_model
#define ACQUIRE gl_StorageSemanticsBuffer, gl_SemanticsAcquire
#define RELEASE gl_StorageSemanticsBuffer, gl_SemanticsRelease
#else
#define ACQUIRE 0, 0
#define RELEASE 0, 0
#endif

#define N_ROWS 16
#define LG_WG_SIZE 9
#define WG_SIZE (1 << LG_WG_SIZE)
#define PARTITION_SIZE (WG_SIZE * N_ROWS)

layout(local_size_x = WG_SIZE, local_size_y = 1) in;

struct Monoid {
    uint element;
};

layout(set = 0, binding = 0) readonly buffer InBuf {
    Monoid[] inbuf;
};

layout(set = 0, binding = 1) buffer OutBuf {
    Monoid[] outbuf;
};

// These correspond to X, A, P respectively in the prefix sum paper.
#define FLAG_NOT_READY 0u
#define FLAG_AGGREGATE_READY 1u
#define FLAG_PREFIX_READY 2u

struct State {
    uint flag;
    Monoid aggregate;
    Monoid prefix;
};

// Perhaps this should be "nonprivate" with VKMM
layout(set = 0, binding = 2) volatile buffer StateBuf {
    uint part_counter;
    State[] state;
};

shared Monoid sh_scratch[WG_SIZE];

Monoid combine_monoid(Monoid a, Monoid b) {
    return Monoid(a.element + b.element);
}

shared uint sh_part_ix;
shared Monoid sh_prefix;
shared uint sh_flag;

void main() {
    Monoid local[N_ROWS];
    // Determine partition to process by atomic counter (described in Section
    // 4.4 of prefix sum paper).
    if (gl_LocalInvocationID.x == 0) {
        sh_part_ix = atomicAdd(part_counter, 1);
    }
    barrier();
    uint part_ix = sh_part_ix;

    uint ix = part_ix * PARTITION_SIZE + gl_LocalInvocationID.x * N_ROWS;

    // TODO: gate buffer read? (evaluate whether shader check or
    // CPU-side padding is better)
    local[0] = inbuf[ix];
    for (uint i = 1; i < N_ROWS; i++) {
        local[i] = combine_monoid(local[i - 1], inbuf[ix + i]);
    }
    Monoid agg = local[N_ROWS - 1];
    sh_scratch[gl_LocalInvocationID.x] = agg;

    for (uint i = 0; i < LG_WG_SIZE; i++) {
        barrier();
        if (gl_LocalInvocationID.x >= (1u << i)) {
            Monoid other = sh_scratch[gl_LocalInvocationID.x - (1u << i)];
            agg = combine_monoid(other, agg);
        }
        barrier();
        sh_scratch[gl_LocalInvocationID.x] = agg;
    }

    // Publish aggregate for this partition
    if (gl_LocalInvocationID.x == WG_SIZE - 1) {
        state[part_ix].aggregate = agg;
        if (part_ix == 0) {
            state[0].prefix = agg;
        }
    }

    // Write flag with release semantics; this is done portably with a barrier.
#ifndef VKMM
    memoryBarrierBuffer();
#endif
    if (gl_LocalInvocationID.x == WG_SIZE - 1) {
        uint flag = FLAG_AGGREGATE_READY;
        if (part_ix == 0) {
            flag = FLAG_PREFIX_READY;
        }
#ifdef ATOMIC
        atomicStore(state[part_ix].flag, flag, gl_ScopeDevice, RELEASE);
#else
        state[part_ix].flag = flag;
#endif
    }

    Monoid exclusive = Monoid(0);
    if (part_ix != 0) {
        // step 4 of paper: decoupled lookback
        uint look_back_ix = part_ix - 1;

        Monoid their_agg;
        uint their_ix = 0;
        while (true) {
            // Read flag with acquire semantics.
            if (gl_LocalInvocationID.x == WG_SIZE - 1) {
#ifdef ATOMIC
                sh_flag = atomicLoad(state[look_back_ix].flag, gl_ScopeDevice, ACQUIRE);
#else
                sh_flag = state[look_back_ix].flag;
#endif
            }
            // The flag load is done only in the last thread. However, because the
            // translation of memoryBarrierBuffer to Metal requires uniform control
            // flow, we broadcast it to all threads.
            barrier();
#ifndef VKMM
            memoryBarrierBuffer();
#endif
            uint flag = sh_flag;
            barrier();

            if (flag == FLAG_PREFIX_READY) {
                if (gl_LocalInvocationID.x == WG_SIZE - 1) {
                    Monoid their_prefix = state[look_back_ix].prefix;
                    exclusive = combine_monoid(their_prefix, exclusive);
                }
                break;
            } else if (flag == FLAG_AGGREGATE_READY) {
                if (gl_LocalInvocationID.x == WG_SIZE - 1) {
                    their_agg = state[look_back_ix].aggregate;
                    exclusive = combine_monoid(their_agg, exclusive);
                }
                look_back_ix--;
                their_ix = 0;
                continue;
            }
            // else spin

            if (gl_LocalInvocationID.x == WG_SIZE - 1) {
                // Unfortunately there's no guarantee of forward progress of other
                // workgroups, so compute a bit of the aggregate before trying again.
                // In the worst case, spinning stops when the aggregate is complete.
                Monoid m = inbuf[look_back_ix * PARTITION_SIZE + their_ix];
                if (their_ix == 0) {
                    their_agg = m;
                } else {
                    their_agg = combine_monoid(their_agg, m);
                }
                their_ix++;
                if (their_ix == PARTITION_SIZE) {
                    exclusive = combine_monoid(their_agg, exclusive);
                    if (look_back_ix == 0) {
                        sh_flag = FLAG_PREFIX_READY;
                    } else {
                        look_back_ix--;
                        their_ix = 0;
                    }
                }
            }
            barrier();
            flag = sh_flag;
            barrier();
            if (flag == FLAG_PREFIX_READY) {
                break;
            }
        }
        // step 5 of paper: compute inclusive prefix
        if (gl_LocalInvocationID.x == WG_SIZE - 1) {
            Monoid inclusive_prefix = combine_monoid(exclusive, agg);
            sh_prefix = exclusive;
            state[part_ix].prefix = inclusive_prefix;
        }
#ifndef VKMM
        memoryBarrierBuffer();
#endif
        if (gl_LocalInvocationID.x == WG_SIZE - 1) {
#ifdef ATOMIC
            atomicStore(state[part_ix].flag, FLAG_PREFIX_READY, gl_ScopeDevice, RELEASE);
#else
            state[part_ix].flag = FLAG_PREFIX_READY;
#endif
        }
    }

    barrier();
    if (part_ix != 0) {
        exclusive = sh_prefix;
    }

    Monoid row = exclusive;
    if (gl_LocalInvocationID.x > 0) {
        Monoid other = sh_scratch[gl_LocalInvocationID.x - 1];
        row = combine_monoid(row, other);
    }
    for (uint i = 0; i < N_ROWS; i++) {
        Monoid m = combine_monoid(row, local[i]);
        // Make sure buffer allocation is padded appropriately.
        outbuf[ix + i] = m;
    }
}
