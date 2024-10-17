#version 450
#extension GL_KHR_shader_subgroup_arithmetic : enable

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0) buffer in_values {
  float values[];
};

layout(set = 0, binding = 1) buffer out_inclusive_prefix_sum {
  float prefix_sum[];
};

void main() {
  uint index = gl_GlobalInvocationID.x;
  prefix_sum[index] = subgroupInclusiveAdd(values[index]);
}

