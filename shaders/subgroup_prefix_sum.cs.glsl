#version 450
#extension GL_KHR_memory_scope_semantics : enable
#pragma use_vulkan_memory_model

#define ACQUIRE gl_StorageSemanticsBuffer, gl_SemanticsAcquire
#define RELEASE gl_StorageSemanticsBuffer, gl_SemanticsRelease

#define N_ROWS 16
#define LG_WG_SIZE 9
#define WG_SIZE (1 << LG_WG_SIZE)
#define PARTITION_SIZE (WG_SIZE * N_ROWS)

layout(local_size_x = WG_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0) readonly buffer in_values {
    float values[];
};

layout(set = 0, binding = 1) buffer out_inclusive_prefix_sum {
    float prefix_sum[];
};

layout(set = 0, binding = 2) volatile buffer atomic_counter {
  uint part_counter;
};

shared float scratch[WG_SIZE];
shared uint sh_part_idx;

void main() {
    float local[N_ROWS];

    if (gl_LocalInvocationID.x == 0) {
      sh_part_idx = atomicAdd(part_counter, 1);
    }
    barrier();
    uint partIdx = sh_part_idx;

    uint idx = partIdx * PARTITION_SIZE + gl_LocalInvocationID.x * N_ROWS;

    local[0] = values[0];
    for (uint i = 1; i < N_ROWS; i++) {
        local[i] = local[i - 1] + values[idx + i];
    }

    float agg = local[N_ROWS - 1];

    scratch[gl_LocalInvocationID.x] = agg;
    for (uint i = 0; i < LG_WG_SIZE; ++i) {
        barrier();
        if (gl_LocalInvocationID.x >= (1u << i)) {
            float other = scratch[gl_LocalInvocationID.x - (1u << i)];
            agg += other;
        }
        barrier();
        scratch[gl_LocalInvocationID.x] = agg;
    }

    barrier();

    float row;
    if (gl_LocalInvocationID.x > 0) {
        row = scratch[gl_LocalInvocationID.x - 1];
    }
    for (uint i = 0; i < N_ROWS; ++i) {
        float m = row + local[i];
        prefix_sum[idx + i] = m;
    }
}

// const uint SUBGROUP_SIZE = gl_WorkGroupSize.x / 32;
//
// shared float subgroupAggregates[SUBGROUP_SIZE];
//
// void main() {
//   uint index = gl_GlobalInvocationID.x;
//   uint invoc = gl_LocalInvocationID.x;
//   uint subInvoc = gl_SubgroupInvocationID.x;
//   uint subID = gl_SubgroupID.x;
//   uint subgroupSize = gl_SubgroupSize.x;
//
//   float item = values[index];
//   float subgroupAggregate = subgroupAdd(item);
//   subgroupAggregates[subID] = subgroupAggregate;
//
//   barrier();
//
//   if (subID == subgroupSize - 1) {
//     float subgroupAgg = (subID < gl_NumSubgroups.x) ? subgroupAggregates[subID] : 0;
//     subgroupMemoryBarrierShared();
//     subgroupAggregates[subInvoc] = subgroupExclusiveAdd(subgroupAgg);
//   }
//   barrier();
//   float subgroupInclusivePrefix = subgroupInclusiveAdd(item);
//
//   prefix_sum[index] = subgroupInclusivePrefix + subgroupAggregates[subID];
// }
//
