#version 450

#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_memory_scope_semantics : enable
#extension GL_EXT_shader_atomic_float : enable
#extension GL_KHR_shader_subgroup_vote : enable
#extension GL_KHR_shader_subgroup_ballot : enable
#pragma use_vulkan_memory_model

#define ROWS 4
#define LOG_GROUP_SIZE 9
#define GROUP_SIZE (1<<LOG_GROUP_SIZE)
#define PARTITION_SIZE (GROUP_SIZE * ROWS)
#define SUBGROUP_SIZE 32
#define SUBGROUP_COUNT (512 / 32)

layout(local_size_x = GROUP_SIZE, local_size_y = 1) in;

#define STATE_NO_AGGREGATE 0u
#define STATE_AGGREGATE 1u
#define STATE_PREFIX 2u
#define STATE_DONT_CARE 3u

struct PartitionDescriptor {
    uint aggregate;
    uint prefix;
    uint state;
};

layout(set = 0, binding = 0) readonly buffer in_values {
    uint values[];
};

layout(set = 0, binding = 1) buffer out_inclusive_prefix_sum {
    uint prefix_sum[];
};

layout(set = 0, binding = 2) volatile buffer parition_descriptors {
    PartitionDescriptor partitions[];
};

layout(set = 0, binding = 3) buffer atomic_workID {
    uint atomicWorkIDCounter;
    uint atomicWorkFinishIdCounter;
};

#define LOOK_BACK_STATE_SPIN 1u
#define LOOK_BACK_STATE_DONE 0u
shared uint lookBackState;
shared uint groupID;
shared uint partPrefix;
shared uint scratch[GROUP_SIZE];

// #define ATOMIC_LOOK_BACK_STATE
// #define ATOMIC_PARTITION_AGGREGATE
#define ATOMIC_PARTITION_PREFIX
// #define ATOMIC_PARTITION_STATE
// #define ATOMIC_LOCAL_PREFIX

// #define ATOMIC_PARTITION_ID

// #define PARALLEL_LOOK_BACK

void main() {
    uint local[ROWS];

    const uint invocID = gl_LocalInvocationID.x;
    const uint subInvocID = gl_SubgroupInvocationID.x;
    const uint subgroupID = gl_SubgroupID.x;
    const uint subgroupCount = gl_NumSubgroups.x;
    const uint subgroupSize = gl_SubgroupSize.x;
    const uint lookBackMaxStepSize = min(subgroupSize, 32);
#ifdef ATOMIC_PARTITION_ID
    if (invocID == 0) {
        groupID = atomicAdd(atomicWorkIDCounter, 1);
        // atomicStore(partitions[groupID].state, STATE_NO_AGGREGATE, gl_ScopeDevice,
        //     gl_StorageSemanticsBuffer, gl_SemanticsRelease);
    }
    controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
    // barrier();
#else
    groupID = gl_WorkGroupID.x;
#endif
    uint partID = groupID;
    uint ix = partID * PARTITION_SIZE + invocID * ROWS;
    // uint itemID = partID * PARTITION_SIZE + invocID * ROWS;
    bool seqInvoc = invocID == gl_WorkGroupSize.x - 1;
    bool seqSubgroup = subgroupID == subgroupCount - 1;


    // 1. Step calculate group aggregate
    local[0] = values[ix];
    for (uint i = 1; i < ROWS; ++i) {
        local[i] = local[i - 1] + values[ix + i];
    }
    uint agg = local[ROWS - 1];

    scratch[invocID] = agg;

    for (uint i = 0; i < LOG_GROUP_SIZE; ++i) {
      controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
      if (invocID >= (1u << i)) {
        uint other = scratch[invocID - (1u << i)];
        agg = agg + other;
      }
      controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
      scratch[invocID] = agg;
    }

    // 2. Publish Aggregate and state
    // 2.1 Publish aggregate
    if (seqInvoc) {
        // atomicStore(partitions[partID].aggregate, aggregate, gl_ScopeWorkgroup, gl_StorageSemanticsBuffer,
        //     gl_SemanticsRelease);
#ifdef ATOMIC_PARTITION_AGGREGATE
        atomicStore(partitions[partID].aggregate, agg, gl_ScopeQueueFamily,
            gl_StorageSemanticsBuffer, gl_SemanticsRelease);
#else
        partitions[partID].aggregate = agg;
#endif
        if (partID == 0) {
#ifdef ATOMIC_PARTITION_PREFIX
            atomicStore(partitions[partID].prefix, agg, gl_ScopeQueueFamily,
                gl_StorageSemanticsBuffer, gl_SemanticsRelease);
#else
            partitions[partID].prefix = agg;
#endif
        }
    }
    // barrier();
    // controlBarrier(gl_ScopeWorkgroup, gl_ScopeDevice, gl_StorageSemanticsBuffer, gl_SemanticsAcquireRelease);

    if (seqInvoc) {
        uint state = partID == 0 ? STATE_PREFIX : STATE_AGGREGATE;
        #ifdef ATOMIC_PARTITION_STATE
        atomicStore(partitions[partID].state, state, gl_ScopeQueueFamily,
            gl_StorageSemanticsBuffer, gl_SemanticsRelease | gl_SemanticsMakeAvailable);
        #else
        partitions[partID].state = state;
        #endif
    }

    controlBarrier(gl_ScopeWorkgroup, gl_ScopeDevice, gl_StorageSemanticsBuffer, gl_SemanticsAcquireRelease);

    // 3. Decoupled Look back
    uint exclusive_prefix = 0;
    if (partID != 0) {
        int previousPartIdx = int(partID) - 1;
        uint s = STATE_NO_AGGREGATE;
        uint cond = LOOK_BACK_STATE_SPIN;
        #ifdef ATOMIC_LOOK_BACK_STATE
        if (seqInvoc) {
            atomicStore(lookBackState, cond,
                gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease);
        }
        #else
        lookBackState = cond;
        #endif
        // controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
        uint lookBackPerfCounter = 0;
        uint exclusive = 0;
        while (cond == LOOK_BACK_STATE_SPIN) {
            lookBackPerfCounter++;

            if (seqSubgroup) {
                bool participant = subInvocID < lookBackMaxStepSize && subInvocID <= previousPartIdx;
                if (participant) {
                    uint subgroupInvocPartIdx = previousPartIdx - subInvocID;
                    #ifdef ATOMIC_PARTITION_STATE
                    s = atomicLoad(partitions[subgroupInvocPartIdx].state, gl_ScopeQueueFamily,
                            gl_StorageSemanticsBuffer, gl_SemanticsAcquire | gl_SemanticsMakeVisible);
                    #else
                    s = partitions[subgroupInvocPartIdx].state;
                    #endif
                } else {
                    s = STATE_DONT_CARE;
                }
                subgroupBarrier();
                bool hasNoAggregate = s == STATE_NO_AGGREGATE;
                bool hasPrefix = s == STATE_PREFIX;
                if (subgroupAny(hasNoAggregate)) {
                    // lookBackState = LOOK_BACK_STATE_SPIN
                } else if (subgroupAny(hasPrefix)) {
                    // uint t = min(lookBackPerfCounter, 9);
                    // item += 1000 * t;
                    uvec4 prefixBallot = subgroupBallot(hasPrefix);
                    uint closestPrefix = subgroupBallotFindLSB(prefixBallot);
                    if (subInvocID <= closestPrefix) {
                        uint v;
                        uint idx = previousPartIdx - subInvocID;
                        if (subInvocID == closestPrefix) {
                            #ifdef ATOMIC_PARTITION_PREFIX
                            v = atomicLoad(partitions[idx].prefix,
                                    gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquire);
                            #else
                            s = partitions[idx].prefix;
                            #endif
                        } else {
                            #ifdef ATOMIC_PARTITION_AGGREGATE
                            v = atomicLoad(partitions[idx].aggregate,
                                    gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquire);
                            #else
                            v = partitions[idx].aggregate;
                            #endif
                        }
                        uint partition_prefix = subgroupAdd(v);
                        if (subgroupElect()) {
                            atomicStore(partPrefix, partition_prefix + exclusive,
                                gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease);
                        }
                    }

                    // sum up everything after the closest part with a prefix
                    if (subgroupElect()) {
                        #ifdef ATOMIC_LOOK_BACK_STATE
                        atomicStore(lookBackState, LOOK_BACK_STATE_DONE,
                            gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease);
                        #else
                        lookBackState = LOOK_BACK_STATE_DONE;
                        #endif
                    }
                } else {
                    uint stepSize = min(previousPartIdx, lookBackMaxStepSize);
                    // uint t = min(lookBackPerfCounter, 9);
                    // item += 10000 * t + 10 * stepSize;

                    if (participant) {
                        uint v;
                        #ifdef ATOMIC_PARTITION_AGGREGATE
                        v = atomicLoad(partitions[previousPartIdx - subInvocID].aggregate,
                                gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquire);
                        #else
                        v = partitions[previousPartIdx - subInvocID].aggregate;
                        #endif
                        uint partition_aggregate = subgroupAdd(v);
                        // uint partition_aggregate = stepSize;
                        exclusive += partition_aggregate;
                    }
                    previousPartIdx -= int(stepSize);
                    // Compute running prefix!
                    if (previousPartIdx < 0) {
                        if (subgroupElect()) {
                            #ifdef ATOMIC_LOCAL_PREFIX
                            atomicStore(partPrefix, exclusive,
                                gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease);
                            #else
                            partPrefix = exclusive;
                            #endif
                        }
                        if (subgroupElect()) {
                            #ifdef ATOMIC_LOOK_BACK_STATE
                            atomicStore(lookBackState, LOOK_BACK_STATE_DONE,
                                gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsRelease);
                            #else
                            lookBackState = LOOK_BACK_STATE_DONE;
                            #endif
                        }
                    }
                }
            }
            controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
#ifdef ATOMIC_LOOK_BACK_STATE
            cond = atomicLoad(lookBackState, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquire);
#else
            cond = lookBackState;
#endif
        }
        // item = lookBackPerfCounter;
//         if (seqInvoc) {
// #ifdef ATOMIC_LOCAL_PREFIX
//             exclusive_prefix = atomicLoad(partPrefix, gl_ScopeWorkgroup,
//                     gl_StorageSemanticsShared, gl_SemanticsAcquire);
// #else
//             exclusive_prefix = partPrefix;
// #endif
//         }
        exclusive_prefix = partPrefix;
        if (seqInvoc) {
#ifdef ATOMIC_PARTITION_PREFIX
            atomicStore(partitions[partID].prefix, exclusive_prefix + agg,
                gl_ScopeQueueFamily, gl_StorageSemanticsBuffer,
                gl_SemanticsRelease);
#else
            partitions[partID].prefix = exclusive_prefix + agg;
#endif
        }

        if (seqInvoc) {
            #ifdef ATOMIC_PARTITION_STATE
            atomicStore(partitions[partID].state, STATE_PREFIX, gl_ScopeQueueFamily,
                gl_StorageSemanticsBuffer, gl_SemanticsRelease);
            #else
            partitions[partID].state = STATE_PREFIX;
            #endif
        }
    }
    // Compute partition inclusive prefix sum
    uint row = exclusive_prefix;
    if (invocID > 0) {
      uint other = scratch[invocID - 1];
      row = row + other;
    }
    for (uint i = 0; i < ROWS; ++i) {
      uint m = row + local[i];
      prefix_sum[ix + i] = m;
    }

    // prefix_sum[itemID] 
}
