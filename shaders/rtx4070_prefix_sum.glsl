#version 450

#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_memory_scope_semantics : enable
// #extension GL_EXT_shader_atomic_float : enable
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

layout(set = 0, binding = 2) buffer parition_descriptors {
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

#define ATOMIC_PARTITION_PREFIX

// #define ATOMIC_PARTITION_ID

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
    }
    controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
#else
    groupID = gl_WorkGroupID.x;
#endif
    uint partID = groupID;
    uint ix = partID * PARTITION_SIZE + invocID * ROWS;
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
        // controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
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
        partitions[partID].aggregate = agg;
        if (partID == 0) {
            atomicStore(partitions[partID].prefix, agg, gl_ScopeQueueFamily,
                gl_StorageSemanticsBuffer, gl_SemanticsRelease);
        }
    }

    if (seqInvoc) {
        uint state = partID == 0 ? STATE_PREFIX : STATE_AGGREGATE;
        partitions[partID].state = state;
    }

    controlBarrier(gl_ScopeWorkgroup, gl_ScopeDevice, gl_StorageSemanticsBuffer, gl_SemanticsAcquireRelease);

    // 3. Decoupled Look back
    uint exclusive_prefix = 0;
    if (partID != 0) {
        int previousPartIdx = int(partID) - 1;
        uint s = STATE_NO_AGGREGATE;
        uint cond = LOOK_BACK_STATE_SPIN;
        lookBackState = cond;
        uint lookBackPerfCounter = 0;
        uint exclusive = 0;
        while (cond == LOOK_BACK_STATE_SPIN) {
            lookBackPerfCounter++;

            if (seqSubgroup) {
                bool participant = subInvocID < lookBackMaxStepSize && subInvocID <= previousPartIdx;
                if (participant) {
                    uint subgroupInvocPartIdx = previousPartIdx - subInvocID;
                    s = partitions[subgroupInvocPartIdx].state;
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
                            v = atomicLoad(partitions[idx].prefix,
                                    gl_ScopeQueueFamily, gl_StorageSemanticsBuffer, gl_SemanticsAcquire);
                        } else {
                            v = partitions[idx].aggregate;
                        }
                        uint partition_prefix = subgroupAdd(v);
                        if (subgroupElect()) {
                            partPrefix = partition_prefix + exclusive;
                        }
                    }

                    // sum up everything after the closest part with a prefix
                    if (subgroupElect()) {
                        lookBackState = LOOK_BACK_STATE_DONE;
                    }
                } else {
                    uint stepSize = min(previousPartIdx, lookBackMaxStepSize);

                    if (participant) {
                        uint v;
                        v = partitions[previousPartIdx - subInvocID].aggregate;
                        uint partition_aggregate = subgroupAdd(v);
                        exclusive += partition_aggregate;
                    }
                    previousPartIdx -= int(stepSize);
                    // Compute running prefix!
                    if (previousPartIdx < 0) {
                        if (subgroupElect()) {
                            partPrefix = exclusive;
                        }
                        if (subgroupElect()) {
                            lookBackState = LOOK_BACK_STATE_DONE;
                        }
                    }
                }
            }
            controlBarrier(gl_ScopeWorkgroup, gl_ScopeWorkgroup, gl_StorageSemanticsShared, gl_SemanticsAcquireRelease);
            cond = lookBackState;
        }
        exclusive_prefix = partPrefix;
        if (seqInvoc) {
            atomicStore(partitions[partID].prefix, exclusive_prefix + agg,
                gl_ScopeQueueFamily, gl_StorageSemanticsBuffer,
                gl_SemanticsRelease);
        }

        if (seqInvoc) {
            partitions[partID].state = STATE_PREFIX;
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
}
