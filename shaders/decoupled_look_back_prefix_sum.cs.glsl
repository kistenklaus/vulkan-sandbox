#version 450

#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_memory_scope_semantics : enable

#pragma use_vulkan_memory_model
#define ACQUIRE gl_StorageSemanticsBuffer, gl_SemanticsAcquire
#define RELEASE gl_StorageSemanticsBuffer, gl_SemanticsRelease

#define GROUP_SIZE 512
#define SUBGROUP_SIZE 32

#define SUBGROUP_COUNT 16

layout(local_size_x = GROUP_SIZE, local_size_y = 1) in;

#define STATE_X 0u
#define STATE_A 1u
#define STATE_P 2u

struct PartitionDescriptor {
    float aggregate;
    float prefix;
    uint state;
};

layout(set = 0, binding = 0) readonly buffer in_values {
    float values[];
};

layout(set = 0, binding = 1) buffer out_inclusive_prefix_sum {
    float prefix_sum[];
};

layout(set = 0, binding = 2) volatile buffer parition_descriptors {
    PartitionDescriptor partitions[];
};

layout(set = 0, binding = 3) buffer atomic_workID {
    uint atomicWorkIDCounter;
};

shared uint groupID;

shared float subgroupAggregates[SUBGROUP_COUNT];
shared float groupPrefix;
shared uint lookBackState;

void main() {
    const uint invocID = gl_LocalInvocationID.x;
    const uint subInvocID = gl_SubgroupInvocationID.x;
    const uint subgroupID = gl_SubgroupID.x;
    if (invocID == 0) {
        groupID = atomicAdd(atomicWorkIDCounter, 1);
    }
    barrier();
    uint partID = groupID;
    uint itemID = partID * GROUP_SIZE + invocID;

    // 1. Step calculate group aggregate
    // - 1.1 Calculate subgroup aggregates
    float item = values[itemID];
    //groupCache[invocID] = values[itemID];

    subgroupAggregates[subgroupID] = subgroupAdd(item);
    barrier();

    // - 1.2 Combine subgroup aggregates.
    float aggregate = 0.0f;
    if (subgroupID == gl_NumSubgroups.x - 1) {
        float subgroupAgg = (subInvocID < gl_NumSubgroups.x) ? subgroupAggregates[subInvocID] : 0;
        aggregate = subgroupAdd(subgroupAgg);
    }
    barrier();

    // 2. Publish aggregate and state
    // 2.1 Write aggregate
    if (invocID == gl_WorkGroupSize.x - 1) {
        partitions[partID].aggregate = aggregate;
        if (partID == 0) {
            partitions[0].prefix = aggregate;
        }
    }
    //memoryBarrierBuffer();
    if (invocID == GROUP_SIZE - 1) {
        uint state = (partID == 0) ? STATE_P : STATE_A;
        atomicStore(partitions[partID].state, state, gl_ScopeDevice, RELEASE);
    }

    float exclusive = 0.0f;
    if (groupID != 0) {
        uint lookBackIndex = partID - 1;
        while (true) {
            if (invocID == GROUP_SIZE - 1) {
                lookBackState = atomicLoad(partitions[lookBackIndex].state, gl_ScopeDevice, ACQUIRE);
            }
            barrier();
            //memoryBarrierBuffer();
            uint state = lookBackState;
            barrier();
            if (state == STATE_P) {
                if (invocID == GROUP_SIZE - 1) {
                    float prefix = partitions[lookBackIndex].prefix;
                    exclusive = exclusive + prefix;
                }
                break;
            } else if (state == STATE_A) {
                if (invocID == GROUP_SIZE - 1) {
                    float agg = partitions[lookBackIndex].aggregate;
                    exclusive = exclusive + agg;
                }
                lookBackIndex--;
                continue;
            }
            // barrier();
            // else spin!
        }

        if (invocID == GROUP_SIZE - 1) {
            float inclusivePrefix = exclusive + aggregate;
            groupPrefix = exclusive;
            partitions[partID].prefix = inclusivePrefix;
        }
        //memoryBarrierBuffer();

        if (invocID == GROUP_SIZE - 1) {
            atomicStore(partitions[partID].state, STATE_P, gl_ScopeDevice, RELEASE);
        }
    }
    barrier();
    memoryBarrierShared();

    float inclusivePrefix = subgroupInclusiveAdd(item);
    if (subgroupID == gl_NumSubgroups.x - 1) {
        float subgroupAgg = (subInvocID < gl_NumSubgroups.x) ? subgroupAggregates[subInvocID] : 0;
        //subgroupMemoryBarrierShared();
        float exclusiveSubgroupPrefix = subgroupExclusiveAdd(subgroupAgg);
        if (subInvocID < gl_NumSubgroups.x) {
            subgroupAggregates[subInvocID] = exclusiveSubgroupPrefix;
        }
    }
    barrier();
    item = subgroupAggregates[subgroupID] + inclusivePrefix + groupPrefix;

    // write back
    prefix_sum[itemID] = item;
}
