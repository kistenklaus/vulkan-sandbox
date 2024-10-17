#include "./Runtime.h"
#include "ext/debug_utils.h"
#include "memory/DeviceMemoryResource.h"
#include <algorithm>
#include <iostream>
#include <logging.h>
#include <memory_resource>
#include <ranges>
#include <stdexcept>
#include <sys/select.h>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

vk::PhysicalDevice
selectPhysicalDevice(vk::Instance instance,
                     std::pmr::memory_resource *bufferResource) {
  using Alloc = std::pmr::polymorphic_allocator<vk::PhysicalDevice>;
  Alloc alloc(bufferResource);
  std::pmr::vector<vk::PhysicalDevice> physicalDevices =
      instance.enumeratePhysicalDevices(alloc, {});
  return *std::ranges::find_if(
      physicalDevices, [](vk::PhysicalDevice physicalDevice) {
        return physicalDevice.getProperties().deviceType ==
               vk::PhysicalDeviceType::eDiscreteGpu;
      });
}

static std::tuple<uint32_t, uint32_t>
selectQueueFamilies(vk::PhysicalDevice physicalDevice,
                    std::pmr::memory_resource *bufferResource) {
  using Alloc = std::pmr::polymorphic_allocator<vk::QueueFamilyProperties>;
  Alloc alloc(bufferResource);

  std::pmr::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      physicalDevice.getQueueFamilyProperties(alloc, {});
  uint32_t graphicsQueueFamilyIndex = -1;
  uint32_t computeQueueFamilyIndex = -1;

  for (auto [i, familyProperty] :
       queueFamilyProperties | std::views::enumerate) {
    if (static_cast<bool>(familyProperty.queueFlags &
                          vk::QueueFlagBits::eGraphics)) {
      graphicsQueueFamilyIndex = i;
    }
    if (static_cast<bool>(familyProperty.queueFlags &
                          vk::QueueFlagBits::eCompute)) {
      computeQueueFamilyIndex = i;
    }
    if (graphicsQueueFamilyIndex != -1 && computeQueueFamilyIndex) {
      break;
    }
  }
  return {graphicsQueueFamilyIndex, computeQueueFamilyIndex};
}

static VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData) {

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    lout::verbose() << lout::VULKAN << data->pMessage << lout::endl();
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    lout::info() << lout::VULKAN << data->pMessage << lout::endl();
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    lout::warning() << lout::VULKAN << data->pMessage << lout::endl();
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    lout::error() << lout::VULKAN << data->pMessage << lout::endl();
    break;
  default:
    lout::error() << lout::VULKAN << data->pMessage << lout::endl();
  }
  return VK_FALSE;
}

using namespace gpu;

Runtime::Runtime(const RuntimeCreateInfo &createInfo) {
  std::byte buffer[256];
  std::pmr::monotonic_buffer_resource bufferResource(buffer, sizeof(buffer));

  vk::ApplicationInfo appInfo;
  appInfo.setApiVersion(VK_API_VERSION_1_3);
  appInfo.setEngineVersion(VK_MAKE_VERSION(0, 0, 0));
  appInfo.setPEngineName("vulkan-sandbox-runtime");
  if (createInfo.applicationName) {
    appInfo.setPApplicationName(createInfo.applicationName->c_str());
  }
  if (createInfo.applicationVersion) {
    appInfo.setApplicationVersion(
        VK_MAKE_VERSION(std::get<0>(*createInfo.applicationVersion),
                        std::get<1>(*createInfo.applicationVersion),
                        std::get<2>(*createInfo.applicationVersion)));
  }
  vk::InstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo.setPApplicationInfo(&appInfo);

  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo;
  debugUtilsCreateInfo.setMessageSeverity(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
  debugUtilsCreateInfo.setMessageType(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding);
  debugUtilsCreateInfo.setPfnUserCallback(debugCallback);

  std::pmr::vector<const char *> extensions(&bufferResource);
  std::pmr::vector<const char *> layers(&bufferResource);
  if (createInfo.validationLayer) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.push_back("VK_LAYER_KHRONOS_validation");
    instanceCreateInfo.setPNext(&debugUtilsCreateInfo);
  }
  instanceCreateInfo.setPEnabledExtensionNames(extensions);
  instanceCreateInfo.setPEnabledLayerNames(layers);

  vk::Result rCreateInstance =
      vk::createInstance(&instanceCreateInfo, nullptr, &instance);
  if (rCreateInstance != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create instance");
  }

  assert(instance);

  if (createInfo.validationLayer) {
    ext::debug_utils::init(instance);
    vk::Result rCreateDebugUtils = instance.createDebugUtilsMessengerEXT(
        &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);

    if (rCreateDebugUtils != vk::Result::eSuccess) {
      throw std::runtime_error("Failed to create debug utils messenger");
    }
  } else {
    debugUtilsMessenger = VK_NULL_HANDLE;
  }

  // Select physical device.
  physicalDevice = selectPhysicalDevice(instance, &bufferResource);
  auto [gfxFamilyIndex, computeFamilyIndex] =
      selectQueueFamilies(physicalDevice, &bufferResource);

  vk::DeviceCreateInfo deviceCreateInfo;
  std::pmr::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{&bufferResource};
  vk::DeviceQueueCreateInfo graphicsQueueCreateInfo;
  float priority = 1.0f;
  graphicsQueueCreateInfo.setQueuePriorities(priority);
  graphicsQueueCreateInfo.setQueueFamilyIndex(gfxFamilyIndex);
  queueCreateInfos.push_back(graphicsQueueCreateInfo);
  if (gfxFamilyIndex != computeFamilyIndex) {
    vk::DeviceQueueCreateInfo computeQueueCreateInfo;
    float priority = 1.0f;
    computeQueueCreateInfo.setQueuePriorities(priority);
    computeQueueCreateInfo.setQueueFamilyIndex(computeFamilyIndex);
    queueCreateInfos.push_back(computeQueueCreateInfo);
  }
  deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);

  vk::PhysicalDeviceFeatures2 features2Available;
  vk::PhysicalDeviceVulkan12Features v12FeaturesAvailable;
  features2Available.setPNext(&v12FeaturesAvailable);
  physicalDevice.getFeatures2(&features2Available);
  std::cout << "VulkanMemoryModel: ";
  std::cout << v12FeaturesAvailable.vulkanMemoryModel << std::endl;

  assert(v12FeaturesAvailable.vulkanMemoryModel);
  assert(v12FeaturesAvailable.vulkanMemoryModelDeviceScope);
  vk::PhysicalDeviceFeatures2 features2;
  vk::PhysicalDeviceVulkan12Features v12Features;
  features2.setPNext(&v12Features);
  v12Features.setVulkanMemoryModelDeviceScope(VK_TRUE);
  v12Features.setVulkanMemoryModel(VK_TRUE);

  deviceCreateInfo.setPNext(&features2);


  vk::Result rCreateDevice =
      physicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);
  if (rCreateDevice != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to create device");
  }

  graphicsQueue = Queue(device, gfxFamilyIndex, 0);
  if (gfxFamilyIndex == computeFamilyIndex) {
    computeQueue = graphicsQueue;
  } else {
    if (createInfo.validationLayer) {
      lout::info() << "GraphicsQueue != ComputeQueue" << lout::endl();
    }
    computeQueue = Queue(device, computeFamilyIndex, 0);
  }

  vk::PhysicalDeviceMemoryProperties memoryProperties =
      physicalDevice.getMemoryProperties();

  uint32_t deviceLocalMemoryTypeIndex = -1;
  uint32_t hostVisibleMemoryTypeIndex = -1;
  for (size_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
    const vk::MemoryType &memoryType = memoryProperties.memoryTypes[i];
    if ((memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) &&
        !(memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eHostVisible)) {
      deviceLocalMemoryTypeIndex = i;
    }
    if (!(memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eDeviceLocal) &&
        (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) {
      hostVisibleMemoryTypeIndex = i;
    }
    if (deviceLocalMemoryTypeIndex != -1 && hostVisibleMemoryTypeIndex != -1) {
      break;
    }
  }
  if (deviceLocalMemoryTypeIndex == -1) {
    throw std::runtime_error(
        "Failed to find memoryType with device local properties");
  }
  if (hostVisibleMemoryTypeIndex == -1) {
    throw std::runtime_error(
        "Failed to find memoryType with host visible properties");
  }
  m_deviceLocalMemory =
      gpu::memory::DeviceMemoryResource(device, deviceLocalMemoryTypeIndex);
  m_hostVisibleMemory =
      gpu::memory::DeviceMemoryResource(device, hostVisibleMemoryTypeIndex);

  if (createInfo.validationLayer) {
    lout::info() << "Memory Types:" << lout::endl();
    for (size_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
      const vk::MemoryType &memoryType = memoryProperties.memoryTypes[i];
      lout::info() << "-" << i << "={";
      if (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eProtected) {
        lout::info() << "PROTECTED," << lout::end();
      }
      if (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached) {
        lout::info() << "HOST_CACHED," << lout::end();
      }
      if (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) {
        lout::info() << "HOST_VISIBLE," << lout::end();
      }
      if (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
        lout::info() << "DEVICE_LOCAL," << lout::end();
      }
      if (memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eHostCoherent) {
        lout::info() << "HOST_COHERENT," << lout::end();
      }
      if (memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eRdmaCapableNV) {
        lout::info() << "RDMA_CAPABLE_NV," << lout::end();
      }
      if (memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eLazilyAllocated) {
        lout::info() << "LAZILY_ALLOCATED," << lout::end();
      }
      if (memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eDeviceUncachedAMD) {
        lout::info() << "DEVICE_UNCACHED_AMD," << lout::end();
      }
      if (memoryType.propertyFlags &
          vk::MemoryPropertyFlagBits::eDeviceCoherentAMD) {
        lout::info() << "DEVICE_COHERENT_AMD," << lout::end();
      }
      lout::info() << "} -> " << memoryType.heapIndex << lout::endl();
    }

    lout::info() << "Memory Heaps:" << lout::endl();
    for (size_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
      vk::MemoryHeap &heap = memoryProperties.memoryHeaps[i];
      lout::info() << "-" << i << "={size=" << heap.size << "," << lout::end();
      if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
        lout::info() << "DEVICE_LOCAL," << lout::end();
      }
      if (heap.flags & vk::MemoryHeapFlagBits::eMultiInstance) {
        lout::info() << "MULTI_INSTANCE," << lout::end();
      }
      if (heap.flags & vk::MemoryHeapFlagBits::eMultiInstanceKHR) {
        lout::info() << "MULTI_INSTANCE_KHR," << lout::end();
      }
      lout::info() << "}" << lout::endl();
    }
  }
}
Runtime::~Runtime() {
  device.destroy();

  if (debugUtilsMessenger) {
    instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
  }
  instance.destroy();
  instance = VK_NULL_HANDLE;
  debugUtilsMessenger = VK_NULL_HANDLE;
}
Runtime::Runtime(Runtime &&o) noexcept
    : instance(std::move(o.instance)),
      physicalDevice(std::move(o.physicalDevice)), device(std::move(o.device)),
      graphicsQueue(std::move(o.graphicsQueue)),
      m_deviceLocalMemory(std::move(o.m_deviceLocalMemory)),
      m_hostVisibleMemory(std::move(o.m_hostVisibleMemory)),
      debugUtilsMessenger(std::move(o.debugUtilsMessenger)) {
  o.instance = VK_NULL_HANDLE;
  o.physicalDevice = VK_NULL_HANDLE;
  o.device = VK_NULL_HANDLE;
  o.m_deviceLocalMemory = std::nullopt;
  o.m_hostVisibleMemory = std::nullopt;
}
Runtime &Runtime::operator=(Runtime &&o) noexcept {
  if (this == &o) {
    return *this;
  }
  this->~Runtime();
  std::swap(instance, o.instance);
  std::swap(physicalDevice, o.physicalDevice);
  std::swap(device, o.device);
  std::swap(graphicsQueue, o.graphicsQueue);
  std::swap(m_deviceLocalMemory, o.m_deviceLocalMemory);
  std::swap(m_hostVisibleMemory, o.m_hostVisibleMemory);
  std::swap(debugUtilsMessenger, o.debugUtilsMessenger);
  return *this;
}
