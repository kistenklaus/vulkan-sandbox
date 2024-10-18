#version 450
#extension GL_KHR_shader_subgroup_arithmetic : enable


layout (local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0) buffer in_values {
  float values[];
};

layout(set = 0, binding = 1) buffer out_inclusive_prefix_sum {
  float prefix_sum[];
};

const uint SUBGROUP_SIZE = gl_WorkGroupSize.x / 32;

shared float subgroupAggregates[SUBGROUP_SIZE];

void main() {
  uint index = gl_GlobalInvocationID.x;
  uint invoc = gl_LocalInvocationID.x;
  uint subInvoc = gl_SubgroupInvocationID.x;
  uint subID = gl_SubgroupID.x;
  uint subgroupSize = gl_SubgroupSize.x;

  float item = values[index];
  float subgroupAggregate = subgroupAdd(item);
  subgroupAggregates[subID] = subgroupAggregate;

  barrier();
  
  if (subID == gl_NumSubgroups.x - 1) {
    float subgroupAgg = (subID < gl_NumSubgroups.x) ? subgroupAggregates[subID] : 0;
    subgroupMemoryBarrierShared();
    subgroupAggregates[subInvoc] = subgroupExclusiveAdd(subgroupAgg);
  }
  barrier();
  float subgroupInclusivePrefix = subgroupInclusiveAdd(item);
  
  prefix_sum[index] = subgroupInclusivePrefix + subgroupAggregates[subID];
}

