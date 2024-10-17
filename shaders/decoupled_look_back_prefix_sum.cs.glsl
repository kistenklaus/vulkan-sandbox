#version 450

#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_memory_scope_semantics : enable

#pragma use_vulkan_memory_model
#define ACQUIRE gl_StorageSemanticsBuffer, gl_SemanticsAcquire
#define RELEASE gl_StorageSemanticsBuffer, gl_SemanticsRelease

#define GROUP_SIZE 64
#define SUBGROUP_SIZE 32

const uint SUBGROUP_COUNT = GROUP_SIZE / SUBGROUP_SIZE;

layout(local_size_x = GROUP_SIZE, local_size_y = 1) in;

#define STATE_X 0
#define STATE_A 1
#define STATE_P 2
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

shared float groupCache[GROUP_SIZE];
shared uint groupID;

shared float subgroupAggregates[SUBGROUP_COUNT];
shared float groupPrefix;
shared uint lookBackState;

void main() {
    uint invocID = gl_LocalInvocationID.x;
    uint subInvocID = gl_SubgroupInvocationID.x;
    uint subgroupID = gl_SubgroupID.x;
    if (invocID == 0) {
        groupID = atomicAdd(atomicWorkIDCounter, 1);
    }
    barrier();
    uint itemID = groupID * GROUP_SIZE + invocID;

    // 1. Step calculate group aggregate
    // - 1.1 Calculate subgroup aggregates
    groupCache[invocID] = values[itemID];
    barrier();

    subgroupAggregates[subgroupID] = subgroupAdd(groupCache[invocID]);
    barrier();
    // - 1.2 Combine subgroup aggregates.
    float aggregate = 0.0f;
    if (invocID == gl_WorkGroupSize.x - 1) {
      for (uint i = 0; i < gl_NumSubgroups.x; ++i) {
          aggregate += subgroupAggregates[i];
      }
    }

    // 2. Publish aggregate and state
    // 2.1 Write aggregate
    if (invocID == gl_WorkGroupSize.x - 1) {
        if (groupID == 0) {
            partitions[groupID].prefix = aggregate;
        } else {
            partitions[groupID].aggregate = aggregate;
        }
    }
    memoryBarrierBuffer();
    if (invocID == gl_WorkGroupSize.x - 1) {
        uint state = groupID == 0 ? STATE_P : STATE_A;
        atomicStore(partitions[groupID].state, state, gl_ScopeDevice, RELEASE);
    }

    float exclusive = 0.0f;
    if (groupID != 0) {
        uint lookBackIndex = groupID - 1;
        while (true) {
            if (invocID == gl_WorkGroupSize.x - 1) {
                lookBackState = atomicLoad(partitions[lookBackIndex].state, gl_ScopeDevice, ACQUIRE);
            }
            barrier();
            memoryBarrierBuffer();
            uint state = lookBackState;
            barrier();
            if (state == STATE_P) {
                if (invocID == gl_WorkGroupSize.x - 1) {
                    float prefix = partitions[lookBackIndex].prefix;
                    exclusive = exclusive + prefix;
                }
                break;
            } else if (state == STATE_A) {
                if (invocID == gl_WorkGroupSize.x - 1) {
                    float agg = partitions[lookBackIndex].aggregate;
                    exclusive = exclusive + agg;
                }
                lookBackIndex--;
                continue;
            }
            // else spin!
        }

        if (invocID == gl_WorkGroupSize.x - 1) {
            groupPrefix = exclusive;
            partitions[groupID].prefix = groupPrefix + aggregate;
        }
        memoryBarrierBuffer();

        if (invocID == gl_WorkGroupSize.x - 1) {
            atomicStore(partitions[groupID].state, uint(STATE_P), gl_ScopeDevice, RELEASE);
        }
    }
    float inclusivePrefix = subgroupInclusiveAdd(groupCache[invocID]);
    // float inclusivePrefix = 0;

    barrier();
    groupCache[invocID] = inclusivePrefix + groupPrefix;

    // write back
    prefix_sum[itemID] = groupCache[invocID];
}
