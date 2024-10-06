#include "CommandPool.h"
#include "Runtime.h"
#include <iostream>
#include <memory_resource>
#include <vulkan/vulkan.hpp>
#include "ComputePipeline.h"
#include "alloc/counting_resource.h"
#include "gpu/memory/MonotonicResource.h"

/* vk::Instance g_instance; */
/* vk::DebugUtilsMessengerEXT g_debugUtils; */
/* vk::PhysicalDevice g_physicalDevice; */
/*  */
/* uint32_t g_computeQueueFamilyIndex; */
/* vk::Queue g_computeQueue; */
/*  */
/* vk::Device g_device; */
/*  */
/* static VkBool32 VKAPI_CALL debugCallback( */
/*     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, */
/*     VkDebugUtilsMessageTypeFlagsEXT messageType, */
/*     const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData) { */
/*  */
/*   std::cout << colorful::VULKAN; */
/*   switch (messageSeverity) { */
/*   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: */
/*     std::cout << colorful::VERBOSE; */
/*     break; */
/*   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: */
/*     std::cout << colorful::INFO; */
/*     break; */
/*   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: */
/*     std::cout << colorful::WARNING; */
/*     break; */
/*   case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: */
/*     std::cout << colorful::ERROR; */
/*     break; */
/*   default: */
/*     std::cout << colorful::ERROR; */
/*   } */
/*   std::cout << data->pMessage; */
/*   std::cout << colorful::RESET << std::endl; */
/*   return VK_FALSE; */
/* } */
/*  */
/* void createInstance() { */
/*   vk::ApplicationInfo appInfo; */
/*   /* appInfo.setPApplicationName("vulkan-sandbox"); */ 
/*   appInfo.setApiVersion(VK_API_VERSION_1_3); */
/*  */
/*   vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo; */
/*   debugUtilsCreateInfo.setMessageSeverity( */
/*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | */
/*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | */
/*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | */
/*       vk::DebugUtilsMessageSeverityFlagBitsEXT::eError); */
/*   debugUtilsCreateInfo.setMessageType( */
/*       vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | */
/*       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | */
/*       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | */
/*       vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding); */
/*   debugUtilsCreateInfo.setPfnUserCallback(debugCallback); */
/*  */
/*   vk::InstanceCreateInfo createInfo; */
/*   createInfo.setPApplicationInfo(&appInfo); */
/*   auto extensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME}; */
/*   createInfo.setPEnabledExtensionNames(extensions); */
/*   auto layers = {"VK_LAYER_KHRONOS_validation"}; */
/*   createInfo.setPEnabledLayerNames(layers); */
/*   createInfo.setPNext(&debugUtilsCreateInfo); */
/*  */
/*   vk::Result r = vk::createInstance(&createInfo, nullptr, &g_instance); */
/*   if (r != vk::Result::eSuccess) { */
/*     throw std::runtime_error("Failed to create instance"); */
/*   } */
/*   ext::debug_utils::init(g_instance); */
/*   vk::Result r2 = g_instance.createDebugUtilsMessengerEXT( */
/*       &debugUtilsCreateInfo, nullptr, &g_debugUtils); */
/*   if (r != vk::Result::eSuccess) { */
/*     throw std::runtime_error("Failed to create debug utils messenger"); */
/*   } */
/* } */
/*  */
/* void destroyInstance() { */
/*   assert(g_instance); */
/*   assert(g_debugUtils); */
/*   g_instance.destroyDebugUtilsMessengerEXT(g_debugUtils); */
/*   g_instance.destroy(); */
/* } */
/*  */
/* void selectPhysicalDevice() { */
/*   auto physicalDevices = g_instance.enumeratePhysicalDevices(); */
/*  */
/*   const std::regex regex("^NVIDIA"); */
/*   for (auto physicalDevice : physicalDevices) { */
/*     const char *deviceName = physicalDevice.getProperties().deviceName; */
/*     ; */
/*     if (std::regex_search(deviceName, regex)) { */
/*       g_physicalDevice = physicalDevice; */
/*       std::cout << colorful::INFO << "Selected Physical Device : " << deviceName */
/*                 << colorful::RESET << std::endl; */
/*       return; */
/*     } */
/*   } */
/*   std::cout << colorful::ERROR; */
/*   std::cout << "Available Physical Devices" << std::endl; */
/*   for (auto physicalDevice : physicalDevices) { */
/*     const char *deviceName = physicalDevice.getProperties().deviceName; */
/*     ; */
/*     std::cout << "-" << deviceName << std::endl; */
/*   } */
/*   std::cout << colorful::RESET; */
/*   throw std::runtime_error("Failed to select physical devices"); */
/* } */
/*  */
/* void selectQueues() { */
/*   auto queueFamilyProperties = g_physicalDevice.getQueueFamilyProperties(); */
/*  */
/*   auto computeQueueRule = */
/*       [](vk::QueueFamilyProperties queueFamilyProperty) -> bool { */
/*     return static_cast<bool>(queueFamilyProperty.queueFlags & */
/*                              vk::QueueFlagBits::eCompute); */
/*   }; */
/*   g_computeQueueFamilyIndex = */
/*       std::ranges::find_if(queueFamilyProperties, computeQueueRule) - */
/*       queueFamilyProperties.begin(); */
/* } */
/*  */
/* void createDevice() { */
/*   selectQueues(); */
/*   vk::DeviceQueueCreateInfo computeQueueCreateInfo; */
/*   computeQueueCreateInfo.setQueueCount(1); */
/*   computeQueueCreateInfo.setQueueFamilyIndex(g_computeQueueFamilyIndex); */
/*   float priority = 1.0f; */
/*   computeQueueCreateInfo.setQueuePriorities(priority); */
/*  */
/*   vk::DeviceCreateInfo createInfo; */
/*   createInfo.setQueueCreateInfos(computeQueueCreateInfo); */
/*  */
/*   vk::Result r = g_physicalDevice.createDevice(&createInfo, nullptr, &g_device); */
/*   if (r != vk::Result::eSuccess) { */
/*     throw std::runtime_error("Failed to create device"); */
/*   } */
/*   g_device.getQueue(g_computeQueueFamilyIndex, 0, &g_computeQueue); */
/* } */
/*  */
/* void destroyDevice() { g_device.destroy(); } */
/*  */
/* int main() { */
/*   { // Create Instance */
/*     createInstance(); */
/*   } */
/*   { // Select Physical Device */
/*     selectPhysicalDevice(); */
/*   } */
/*   { // Create Device */
/*     createDevice(); */
/*   } */
/*  */
/*   vk::CommandPool commandPool; */
/*   { // Create Command Pool */
/*     vk::CommandPoolCreateInfo commandPoolCreateInfo; */
/*     commandPoolCreateInfo.setQueueFamilyIndex(g_computeQueueFamilyIndex); */
/*     vk::Result r = g_device.createCommandPool(&commandPoolCreateInfo, nullptr, */
/*                                               &commandPool); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create command pool"); */
/*     } */
/*     std::cout << colorful::INFO << "create CommandPool: " << commandPool */
/*               << colorful::RESET << std::endl; */
/*   } */
/*  */
/*   vk::CommandBuffer commandBuffer; */
/*   { // Allocate Command buffer */
/*     vk::CommandBufferAllocateInfo allocInfo; */
/*     allocInfo.setCommandPool(commandPool); */
/*     allocInfo.commandBufferCount = 1; */
/*     vk::Result r = g_device.allocateCommandBuffers(&allocInfo, &commandBuffer); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to allocate command buffer"); */
/*     } */
/*     std::cout << colorful::INFO << "allocated commandBuffer: " << commandBuffer */
/*               << colorful::RESET << std::endl; */
/*   } */
/*  */
/*   vk::ShaderModule computeShader; */
/*   { // Create Shader Module */
/*     vk::ShaderModuleCreateInfo createInfo; */
/*     std::string filename = "./shaders/basic.cs.spv"; */
/*     std::vector<std::byte> code = io::readFileSync(filename); */
/*     std::string filename2 = "./shaders/basic.cs.glsl"; */
/*  */
/*     std::vector<std::byte> code2 = io::readFileSync(filename2); */
/*     std::vector<uint32_t> spirvCode = spirv::compileShader(code2, filename2); */
/*     std::cout << spirvCode.size() << " " << code.size() << std::endl; */
/*     /* assert(spirvCode.size() == code.size()); */ 
/*  */
/*     createInfo.setCode(spirvCode); */
/*  */
/*     /* createInfo.setCode({static_cast<uint32_t>(code.size()), */
/*      * reinterpret_cast<const uint32_t*>(code.data())}); */ 
/*  */
/*     vk::Result r = */
/*         g_device.createShaderModule(&createInfo, nullptr, &computeShader); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create shader module"); */
/*     } */
/*     std::cout << colorful::INFO << "created computeShader: " << computeShader */
/*               << colorful::RESET << std::endl; */
/*   } */
/*  */
/*   vk::DescriptorSetLayout descriptorSetLayout; */
/*   { */
/*     vk::DescriptorSetLayoutCreateInfo createInfo; */
/*     vk::DescriptorSetLayoutBinding outputBinding; */
/*     outputBinding.setBinding(0); */
/*     outputBinding.setDescriptorType(vk::DescriptorType::eStorageBuffer); */
/*     outputBinding.setStageFlags(vk::ShaderStageFlagBits::eCompute); */
/*     outputBinding.setDescriptorCount(1); */
/*     createInfo.setBindings(outputBinding); */
/*     vk::Result r = g_device.createDescriptorSetLayout(&createInfo, nullptr, */
/*                                                       &descriptorSetLayout); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create descriptor set layout"); */
/*     } */
/*     std::cout << colorful::INFO */
/*               << "created descriptorSetLayout: " << descriptorSetLayout */
/*               << colorful::RESET << std::endl; */
/*   } */
/*  */
/*   vk::PipelineLayout computePipelineLayout; */
/*   { // Create Pipeline Layout */
/*     vk::PipelineLayoutCreateInfo createInfo; */
/*     createInfo.setSetLayouts(descriptorSetLayout); */
/*     vk::Result r = g_device.createPipelineLayout(&createInfo, nullptr, */
/*                                                  &computePipelineLayout); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create pipeline layout"); */
/*     } */
/*     std::cout << colorful::INFO */
/*               << "created computePipelineLayout: " << computePipelineLayout */
/*               << colorful::RESET << std::endl; */
/*   } */
/*  */
/*   vk::Pipeline computePipeline; */
/*   { // Create compute queue */
/*     vk::ComputePipelineCreateInfo createInfo; */
/*     createInfo.stage.setModule(computeShader); */
/*     createInfo.stage.setStage(vk::ShaderStageFlagBits::eCompute); */
/*     createInfo.stage.setPName("main"); */
/*     createInfo.setLayout(computePipelineLayout); */
/*     vk::Result r = g_device.createComputePipelines(nullptr, 1, &createInfo, */
/*                                                    nullptr, &computePipeline); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create compute pipeline"); */
/*     } */
/*   } */
/*  */
/*   { // Destroy Shader Module */
/*     g_device.destroyShaderModule(computeShader); */
/*   } */
/*  */
/*   std::vector<float> computeInput(128,0); */
/*   vk::Buffer ssboBuffer; */
/*   { // Create Shader Storage Buffer Object (SSBO) */
/*     vk::BufferCreateInfo bufferCreateInfo; */
/*     bufferCreateInfo.setSharingMode(vk::SharingMode::eExclusive); */
/*     bufferCreateInfo.setSize(computeInput.size() * sizeof(float)); */
/*     bufferCreateInfo.setUsage(vk::BufferUsageFlagBits::eStorageBuffer | */
/*                               vk::BufferUsageFlagBits::eTransferDst); */
/*     vk::Result r = */
/*         g_device.createBuffer(&bufferCreateInfo, nullptr, &ssboBuffer); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create ssbo buffer"); */
/*     } */
/*   } */
/*   vk::DeviceMemory ssboMemory; */
/*   { // Allocate SSBO buffer. */
/*     vk::MemoryRequirements memReq = */
/*         g_device.getBufferMemoryRequirements(ssboBuffer); */
/*     vk::PhysicalDeviceMemoryProperties deviceMemoryProperties = */
/*         g_physicalDevice.getMemoryProperties(); */
/*     // Find memoryType */
/*     auto properties = vk::MemoryPropertyFlagBits::eHostVisible | */
/*                       vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal; */
/*     int memoryTypeIndex = -1; */
/*     for (size_t i = 0; i < deviceMemoryProperties.memoryTypes.size(); ++i) { */
/*       if (memReq.memoryTypeBits & (1 << i) && */
/*           (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == */
/*               properties) { */
/*         memoryTypeIndex = i; */
/*         break; */
/*       } */
/*     } */
/*     if (memoryTypeIndex == -1) { */
/*       throw std::runtime_error("Faild to find memory for ssbo buffer"); */
/*     } */
/*     vk::MemoryAllocateInfo allocInfo; */
/*     allocInfo.setAllocationSize(memReq.size); */
/*     allocInfo.setMemoryTypeIndex(memoryTypeIndex); */
/*     vk::Result r = g_device.allocateMemory(&allocInfo, nullptr, &ssboMemory); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to allocate ssbo memory"); */
/*     } */
/*   } */
/*   { // Bind SSBO Memory */
/*     g_device.bindBufferMemory(ssboBuffer, ssboMemory, 0); */
/*   } */
/*  */
/*   { // Upload computeInput to ssboBuffer */
/*     void *mappedMemory; */
/*     vk::Result r = */
/*         g_device.mapMemory(ssboMemory, 0, computeInput.size() * sizeof(float), */
/*                            vk::MemoryMapFlagBits(0), &mappedMemory); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to map ssbo memory to host."); */
/*     } */
/*     std::memcpy(mappedMemory, computeInput.data(), */
/*                 computeInput.size() * sizeof(float)); */
/*     g_device.unmapMemory(ssboMemory); */
/*   } */
/*  */
/*   vk::DescriptorPool descriptorPool; */
/*   { // Create Descriptor Pool */
/*     vk::DescriptorPoolCreateInfo createInfo; */
/*     vk::DescriptorPoolSize poolSize; */
/*     poolSize.descriptorCount = 1; */
/*     createInfo.setPoolSizes(poolSize); */
/*     createInfo.setMaxSets(1); */
/*     vk::Result r = */
/*         g_device.createDescriptorPool(&createInfo, nullptr, &descriptorPool); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create descriptor pool"); */
/*     } */
/*   } */
/*   vk::DescriptorSet descriptorSet; */
/*   { // Allocate Descriptor Set */
/*     vk::DescriptorSetAllocateInfo allocInfo; */
/*     allocInfo.setDescriptorPool(descriptorPool); */
/*     allocInfo.setDescriptorSetCount(1); */
/*     allocInfo.setSetLayouts(descriptorSetLayout); */
/*     vk::Result r = g_device.allocateDescriptorSets(&allocInfo, &descriptorSet); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to allocate descriptor set"); */
/*     } */
/*   } */
/*   { // Configure Descriptor Set */
/*     vk::WriteDescriptorSet writeInfo; */
/*     writeInfo.setDstSet(descriptorSet); */
/*     writeInfo.setDstBinding(0); */
/*     writeInfo.setDstArrayElement(0); */
/*     writeInfo.setDescriptorType(vk::DescriptorType::eStorageBuffer); */
/*  */
/*     vk::DescriptorBufferInfo bufferInfo; */
/*     bufferInfo.buffer = ssboBuffer; */
/*     bufferInfo.offset = 0; */
/*     bufferInfo.range = computeInput.size() * sizeof(float); */
/*     writeInfo.setBufferInfo(bufferInfo); */
/*     g_device.updateDescriptorSets(1, &writeInfo, 0, nullptr); */
/*   } */
/*  */
/*   vk::Fence fence; */
/*   { // Create Fence */
/*     vk::FenceCreateInfo createInfo; */
/*     vk::Result r = g_device.createFence(&createInfo, nullptr, &fence); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create fence"); */
/*       ; */
/*     } */
/*   } */
/*   vk::Semaphore semaphore; */
/*   { // Create semaphore */
/*     vk::SemaphoreCreateInfo createInfo; */
/*     vk::Result r = g_device.createSemaphore(&createInfo, nullptr, &semaphore); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to create semaphore"); */
/*     } */
/*   } */
/*  */
/*   { // Begin CommandBuffer */
/*     vk::CommandBufferBeginInfo beginInfo; */
/*     vk::Result r = commandBuffer.begin(&beginInfo); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to begin command buffer"); */
/*     } */
/*   } */
/*  */
/*   { // Bind Pipeline */
/*     commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, */
/*                                computePipeline); */
/*   } */
/*  */
/*   { // Bind Descriptor Set */
/*     commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, */
/*                                      computePipelineLayout, 0, 1, */
/*                                      &descriptorSet, 0, nullptr); */
/*   } */
/*  */
/*   { // Dispatch Compute Pipeline */
/*     commandBuffer.dispatch(1000, 1, 1); */
/*   } */
/*   { // Memory barrier */
/*     vk::BufferMemoryBarrier barrier; */
/*     barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite; */
/*     barrier.dstAccessMask = vk::AccessFlagBits::eHostRead; */
/*     barrier.buffer = ssboBuffer; */
/*     barrier.size = computeInput.size() * sizeof(float); */
/*     commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eHost, */
/*         vk::DependencyFlags(), */
/*         nullptr, */
/*         barrier, */
/*         nullptr); */
/*      */
/*   } */
/*  */
/*   { // End Command Buffer */
/*     commandBuffer.end(); */
/*   } */
/*  */
/*   { // Submit Command Buffer */
/*     vk::SubmitInfo submitInfo; */
/*     submitInfo.setCommandBuffers(commandBuffer); */
/*     submitInfo.setSignalSemaphores(semaphore); */
/*     g_computeQueue.submit(submitInfo, fence); */
/*   } */
/*  */
/*   { // Wait for Fence */
/*     vk::Result r = g_device.waitForFences(1, &fence, true, UINT64_MAX); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to wait for fence"); */
/*     } */
/*   } */
/*   g_device.waitIdle(); */
/*  */
/*   { // Download computeInput to ssboBuffer */
/*     void *mappedMemory; */
/*     std::cout << "computeInput.size() = " << computeInput.size() << std::endl; */
/*     vk::Result r = */
/*         g_device.mapMemory(ssboMemory, 0, computeInput.size() * sizeof(float), */
/*                            vk::MemoryMapFlagBits(0), &mappedMemory); */
/*     if (r != vk::Result::eSuccess) { */
/*       throw std::runtime_error("Failed to map ssbo memory to host."); */
/*     } */
/*     std::memcpy(computeInput.data(), mappedMemory, */
/*                 computeInput.size() * sizeof(float)); */
/*     g_device.unmapMemory(ssboMemory); */
/*   } */
/*   { // Print Output */
/*     for (auto [i, v] : computeInput | std::views::enumerate) { */
/*       std::cout << "out[" << i << "] = " << v << std::endl; */
/*     } */
/*   } */
/*  */
/*   std::cout << "Hello, World!" << std::endl; */
/*  */
/*   { // Free Descriptor Set */
/*     // NOTE note they get freed when the pool gets destroyed */
/*     /* g_device.freeDescriptorSets(descriptorPool, descriptorSet); */ 
/*   } */
/*  */
/*   { // Destroy DescriptorPool */
/*     g_device.destroyDescriptorPool(descriptorPool); */
/*   } */
/*  */
/*   { // Free SSBO Buffer */
/*     g_device.freeMemory(ssboMemory); */
/*   } */
/*  */
/*   { // Destroy SSBO Buffer */
/*     g_device.destroyBuffer(ssboBuffer); */
/*   } */
/*  */
/*   { // Destroy Seamphore */
/*     g_device.destroySemaphore(semaphore); */
/*   } */
/*  */
/*   { // Destroy Fence */
/*     g_device.destroyFence(fence); */
/*   } */
/*  */
/*   { // Destroy Pipeline */
/*     g_device.destroyPipeline(computePipeline); */
/*   } */
/*  */
/*   { // Destroy Pipeline Layout */
/*     g_device.destroyPipelineLayout(computePipelineLayout); */
/*   } */
/*  */
/*   { // Destroy DescriptorSetLayout */
/*     g_device.destroyDescriptorSetLayout(descriptorSetLayout); */
/*   } */
/*  */
/*   { // Free Command buffer */
/*     g_device.freeCommandBuffers(commandPool, commandBuffer); */
/*   } */
/*  */
/*   { // Destroy Command Pool */
/*     g_device.destroyCommandPool(commandPool); */
/*   } */
/*   { // Destroy Device */
/*     destroyDevice(); */
/*   } */
/*  */
/*   { // Destroy Instance */
/*     destroyInstance(); */
/*   } */
/* } */

/* int main () { */
/*   CountingMemoryResource countingResource; */
/*  */
/*   std::pmr::monotonic_buffer_resource bufferResource{&countingResource}; */
/*  */
/*   RuntimeCreateInfo runtimeCreateInfo; */
/*   Runtime runtime(runtimeCreateInfo); */
/*   { */
/*  */
/*     gpu::memory::MonotonicResource bufferMemoryResource(&runtime.deviceLocalMemory, 4096 * 100); */
/*  */
/*     pmr::ComputePipeline computeShader(runtime, "./shaders/basic.cs.glsl", 1, &bufferResource, &bufferResource); */
/*     pmr::CommandPool pool(runtime, {}, &bufferResource); */
/*     CommandBuffer commandBuffer = pool.allocate(); */
/*  */
/*     /* DescriptorSet set = computeShader.allocateDescriptorSet(0); */ */
/*     /* set.write(0, vk::Buffer buffer); */ */
/*  */
/*     /* commandBuffer.begin(); */ */
/*  */
/*     /* commandBuffer.end(); */ */
/*  */
/*     /* vk::SubmitInfo submitInfo; */ */
/*     /* runtime.graphicsQueue.submit(submitInfo); */ */
/*  */
/*   } */
/*  */
/*   std::cout << "count=" << countingResource.allocCount() << std::endl; */
/* } */
/*  */

