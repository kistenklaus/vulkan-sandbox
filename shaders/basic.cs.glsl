#version 450

layout(set = 0, binding = 1) buffer X {
  float y[];
};

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
void main() {
  // uint index = gl_GlobalInvocationID.x;
  //
  // outputSSBO[index] = gl_LocalInvocationID.x;
}
