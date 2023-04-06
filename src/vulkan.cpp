
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <chrono>
#include <set>

// header only library
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp> // only for debug, printing vectors

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STD_IMAGE_IMPLEMENTATION

#include "vulkan.h"
#include "camera.h"

struct UniformBufferObject {
  UniformBufferObject() = default;
  UniformBufferObject(glm::mat4 m, glm::mat4 v, glm::mat4 p): model(m), view(v), projection(p) {}

public:
  // shader requirements; offsets must be multiples of 16 
  // (glsl mat4 requires same alignment as vec3/vec4)
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
};

struct VulkanVertex {
  VulkanVertex() = default;
  VulkanVertex(glm::vec3 p, glm::vec3 c, glm::vec2 t): pos(p), col(c), tex(t) {}

public:
  static auto get_vulkan_binding_description() -> VkVertexInputBindingDescription {
    VkVertexInputBindingDescription binding_description{};
    // index of binding in array of bindings (all data packed in one array, only using one binding)
    binding_description.binding = 0;
    binding_description.stride = sizeof(VulkanVertex); // number of bytes from one entry to next
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return binding_description;
  }

  static auto get_vulkan_attribute_descriptions() -> std::array<VkVertexInputAttributeDescription, 3> {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
    attribute_descriptions[0].binding = 0; // index from which binding per-vertex data comes
    // references location directive of input in vertex shader
    // input in location 0 is position, with three 32-bit float components
    attribute_descriptions[0].location = 0; // *** layout(location = 0)
    // format describes type of data for attribute, some potential formats;
    // - float: VK_FORMAT_R32_SFLOAT
    // - double: VK_FORMAT_R64_SFLOAT
    // - vec2: VK_FORMAT_R32G32_SFLOAT
    // - vec3: VK_FORMAT_R32G32B32_SFLOAT
    // - vec4: VK_FORMAT_R32G32B32A32_SFLOAT
    // - ivec2: VK_FORMAT_R32G32_SINT
    // - uvec4: VK_FORMAT_R32G32B32A32_UINT
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(VulkanVertex, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1; // *** layout(location = 1)
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(VulkanVertex, col);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2; // *** layout(location = 2)
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(VulkanVertex, tex);

    return attribute_descriptions;
  }

public:
  glm::vec3 pos;
  glm::vec3 col;
  glm::vec2 tex;
};

/*
// without index buffer
const std::vector<VulkanVertex> vulkan_vertices = {
  //      POS              COL
  VulkanVertex({ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}),
  VulkanVertex({ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}),
  VulkanVertex({-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f})
};
*/
// with index buffer
//
// 0------------1
// |\..         |
// |   \..      | 
// |      \..   |
// |         \..|
// 3------------2
//
// forms 2 vulkans, one with {0, 1, 3}, the other with {2, 3, 0}
const std::vector<VulkanVertex> vulkan_vertices = {
  VulkanVertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}),
  VulkanVertex({ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}),
  VulkanVertex({ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}),
  VulkanVertex({-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f})
};
// can use uint16_t when < 65535 unique vertices, can use uint32_t when over (will need to specify choice later)
const std::vector<uint16_t> vulkan_indices = {
  0, 1, 2, 2, 3, 0
};

struct QueueFamilyIndices {
  QueueFamilyIndices() = default;

public:
  auto is_complete() -> bool {
    return graphics_family.has_value() && present_family.has_value();
  }

public:
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;
};

struct SwapChainSupportDetails {
  SwapChainSupportDetails() = default;

public:
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

static auto check_validation_layer_support(void) -> bool;
static auto message_callback_get_required_extensions(void) -> std::vector<const char*>;
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);
static auto populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT&) -> void;
static auto choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>&) -> VkSurfaceFormatKHR;
static auto choose_swap_present_mode(const std::vector<VkPresentModeKHR>&) -> VkPresentModeKHR;
static auto choose_swap_extent(GLFWwindow*, const VkSurfaceCapabilitiesKHR&) -> VkExtent2D;
static auto read_file(const std::string&) -> std::vector<char>;
static auto create_shader_module(VkDevice, const std::vector<char>&) -> VkShaderModule;
static auto framebuffer_resize_callback(GLFWwindow*, int width, int height) -> void;

namespace vulkan {

// ---- Main Application Pipeline ----
auto VulkanApplication::run(void) -> void {
  init_window();
  init_vulkan();
  main_loop();
  cleanup();
}

auto VulkanApplication::init_window(void) -> void {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanApplication", nullptr, nullptr);
  if(window == nullptr)
    throw std::runtime_error("Error - (window) is nullptr");

  glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

  // optional user pointer for the window, currently points to its owning object
  glfwSetWindowUserPointer(window, this);

  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods){
    const auto& key_callbacks = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window))->key_callbacks;
    for(const auto& callback : key_callbacks)
      callback(window, key, scancode, action, mods);
  });

  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x_pos, double y_pos){
    const auto& cursor_callbacks = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window))->cursor_callbacks;
    for(const auto& callback : cursor_callbacks)
      callback(window, x_pos, y_pos);
  });

  // close window
  add_key_callback([](GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
  });

  // move main_camera
  add_key_callback([](GLFWwindow* window, int key, int scancode, int action, int mods){
    const auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    auto& camera = app->main_camera;
    auto delta_time = app->glfw_delta_time;

    float camera_move_speed = 20.0f;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.move({0.0f, 0.0f, camera_move_speed * delta_time});

    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.move({camera_move_speed * delta_time, 0.0f, 0.0f});

    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.move({0.0f, 0.0f, -camera_move_speed * delta_time});

    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.move({-camera_move_speed * delta_time, 0.0f, 0.0f});

    // y-axis is inverted compared to OpenGL
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      camera.move({0.0f, -1*camera_move_speed * delta_time, 0.0f});

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      camera.move({0.0f, -1*-camera_move_speed * delta_time, 0.0f});
  });

  // set cursor position
  add_cursor_callback([](GLFWwindow* window, double x_pos, double y_pos){
    auto& [current_x, current_y] = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window))->cursor_pos;
    current_x = x_pos;
    current_y = y_pos;
  });
}

auto VulkanApplication::init_vulkan(void) -> void {
  // creation order matters, as some functions are reliant on class members that must be initialized prior
  create_instance();
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
  create_image_views();
  create_render_pass();
  create_descriptor_set_layout();
  create_descriptor_pool();
  create_graphics_pipeline();
  create_framebuffers();
  create_command_pool();
  create_texture_image();
  create_texture_image_view();
  create_texture_sampler();
  create_vertex_buffer();
  create_index_buffer();
  create_uniform_buffers();
  create_descriptor_sets(); // created with other descriptor stuff for coherence, but relies on uniforms created above
  create_command_buffers();
  create_sync_objects();
}

auto VulkanApplication::main_loop(void) -> void {
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    update_glfw_delta_time();
    draw_frame();
  }
  vkDeviceWaitIdle(device);
}

auto VulkanApplication::cleanup(void) -> void {
  // vulkan cleanup
  cleanup_swap_chain();

  vkDestroySampler(device, texture_sampler, nullptr);
  vkDestroyImageView(device, texture_image_view, nullptr);

  vkDestroyImage(device, texture_image, nullptr);
  vkFreeMemory(device, texture_image_memory, nullptr);

  for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(device, uniform_buffers[i], nullptr);
    vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
  }
  vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

  vkDestroyBuffer(device, vertex_buffer, nullptr);
  vkFreeMemory(device, vertex_buffer_memory, nullptr);

  vkDestroyBuffer(device, index_buffer, nullptr);
  vkFreeMemory(device, index_buffer_memory, nullptr);

  for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device, semaphores_image_available_render[i], nullptr);
    vkDestroySemaphore(device, semaphores_render_finished_present[i], nullptr);
    vkDestroyFence(device, fences_in_flight[i], nullptr);
  }

  vkDestroyCommandPool(device, command_pool, nullptr);

  // SO; WEIRD THING HAPPENS WITH LAYERS ON SOME ANDROID/WINDOWS DEVICES (AFAIK)
  // when validation layers are enabled, it can cause an error to pop up regarding
  // child objects that have yet to be destroyed before the device is (namely the
  // 3 objects below this comment). when validations layers are disabled, the
  // warning goes away.
  // per; https://stackoverflow.com/questions/61273270/vulkan-validation-error-for-each-objects-when-destroying-device-despite-their-d
  vkDestroyPipeline(device, graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  vkDestroyRenderPass(device, render_pass, nullptr);

  vkDestroyDevice(device, nullptr);

  if(enable_validation_layers)
    destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  // glfw cleanup
  glfwDestroyWindow(window);
  glfwTerminate();
}
// ---- End of Main Application Pipeline ----

// ---- Setup ----
auto VulkanApplication::get_window_user_ptr(void) const -> void* {
  return glfwGetWindowUserPointer(window);
}

auto VulkanApplication::add_key_callback(KeyCallback key_callback) -> void {
  key_callbacks.push_back(std::move(key_callback));
}

auto VulkanApplication::add_cursor_callback(CursorCallback cursor_callback) -> void {
  cursor_callbacks.push_back(std::move(cursor_callback));
}

auto VulkanApplication::create_instance(void) -> void {
  if(enable_validation_layers && !check_validation_layer_support())
    throw std::runtime_error("Error - validation layers requested, but unavailable");

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "VulkanApplication";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  auto extensions = message_callback_get_required_extensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();
  create_info.enabledLayerCount = 0;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
  if(enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
    populate_debug_messenger_create_info(debug_create_info);
    // according to the standard, pNext should be set to nullptr in header v70
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create instance");
}

auto VulkanApplication::setup_debug_messenger(void) -> void {
  if(!enable_validation_layers) return;

  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  populate_debug_messenger_create_info(create_info);

  if(create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to set up debug messenger");
}

auto VulkanApplication::pick_physical_device(void) -> void {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if(device_count == 0)
    throw std::runtime_error("Error - failed to find GPU(s) with Vulkan support");

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  for(const auto& device : devices) {
    if(is_device_suitable(device)) {
      physical_device = device;
      break;
    }
  }

  if(physical_device == VK_NULL_HANDLE)
    throw std::runtime_error("Error - failed to find suitable GPU(s)");
}

auto VulkanApplication::create_logical_device(void) -> void {
  QueueFamilyIndices indices = find_queue_families(physical_device);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  auto unique_queue_families = std::set<uint32_t>{indices.graphics_family.value(), indices.present_family.value()};

  auto queue_priority = 1.f;
  for(const auto queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};
  device_features.samplerAnisotropy = VK_TRUE; // anisotropic filtering enabled!

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();

  create_info.pEnabledFeatures = &device_features;

  create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
  create_info.ppEnabledExtensionNames = device_extensions.data();

  if(enable_validation_layers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }

  if(vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create logical device");

  vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
  vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

auto VulkanApplication::create_surface(void) -> void {
  if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create window surface");
}

auto VulkanApplication::create_swap_chain(void) -> void {
  auto swap_chain_support = query_swap_chain_support(physical_device);

  VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
  VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
  VkExtent2D extent = choose_swap_extent(window, swap_chain_support.capabilities);

  // specifying minimum may cause interal delays, so request min+1
  auto image_count = swap_chain_support.capabilities.minImageCount + 1;

  if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    image_count = swap_chain_support.capabilities.maxImageCount;
  
  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;
  // swap chain details
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = find_queue_families(physical_device);
  uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0; // optional
    create_info.pQueueFamilyIndices = nullptr; // optional
  }

  create_info.preTransform = swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  if(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create swap chain");

  vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
  swap_chain_images.resize(image_count);
  vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

  swap_chain_image_format = surface_format.format;
  swap_chain_extent = extent;
}

auto VulkanApplication::create_image_views(void) -> void {
  auto image_count = swap_chain_images.size();
  swap_chain_image_views.resize(image_count);

  for(std::size_t i = 0; i < image_count; ++i) {
    swap_chain_image_views[i] = create_image_view(swap_chain_images[i], swap_chain_image_format);
  }
}

auto VulkanApplication::create_render_pass(void) -> void {
  VkAttachmentDescription colour_attachment{};
  colour_attachment.format = swap_chain_image_format;
  colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colour_attachment_ref{};
  colour_attachment_ref.attachment = 0;
  colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // VK_SUBPASS_EXTERNAL refers to implicit subpasses
  dependency.dstSubpass = 0; // refers to our subpass, which is first and only one
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // operation to wait on
  dependency.srcAccessMask = 0; // stages in which the operation occurs
  // wait for the swap chain to finish reading from the image before can access it
  // -> wait on color attachment output stage itself
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; 
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &colour_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  if(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create render pass");
}

auto VulkanApplication::create_descriptor_set_layout(void) -> void {
  // uniform bindings
  VkDescriptorSetLayoutBinding ubo_layout_binding{};
  ubo_layout_binding.binding = 0; // *** layout(binding = 0)
  ubo_layout_binding.descriptorCount = 1; // number of values in the array (only 1 struct in the shader)
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.pImmutableSamplers = nullptr;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // could also be VK_SHADER_STAGE_ALL_GRAPHICS

  // create another binding for combined image sampler
  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding = 1;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding, sampler_layout_binding};

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = bindings.size();
  layout_info.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create descriptor set layout");
}

auto VulkanApplication::create_descriptor_pool(void) -> void {
  std::array<VkDescriptorPoolSize, 2> pool_sizes{};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // set binding in create_descriptor_set_layout
  pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // set binding in create_descriptor_set_layout
  pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();
  pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create descriptor pool");
}

auto VulkanApplication::create_descriptor_sets(void) -> void {
  descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool;
  alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // must be size of descriptor_sets
  alloc_info.pSetLayouts = layouts.data();
  
  if(vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets!");

  for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffers[i];
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture_image_view;
    image_info.sampler = texture_sampler;

    std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = descriptor_sets[i];
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo = &buffer_info;

    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet = descriptor_sets[i];
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pImageInfo = &image_info;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
  }
}

auto VulkanApplication::create_graphics_pipeline(void) -> void {
  // src/vulkan.cpp -> shaders/vert.spv & shaders/frag.spv
  auto vertex_shader_bytecode   = read_file("../shaders/vert.spv");
  auto fragment_shader_bytecode = read_file("../shaders/frag.spv");

  auto vertex_shader = create_shader_module(device, vertex_shader_bytecode);
  auto fragment_shader = create_shader_module(device, fragment_shader_bytecode);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vertex_shader;
  vert_shader_stage_info.pName = "main"; // shader entry point -> can have different if so choose

  VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = fragment_shader;
  frag_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
  auto binding_description = VulkanVertex::get_vulkan_binding_description();
  auto attribute_descriptions = VulkanVertex::get_vulkan_attribute_descriptions();

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions = &binding_description; // optional
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); // optional

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  // both viewport and scissor are dynamic states
  // would use the following setup if they were static states
  /*
  VkViewport viewport{};
  viewport.x = 0.f;
  viewport.y = 0.f;
  viewport.width = (float) swap_chain_extent.width;
  viewport.height = (float) swap_chain_extent.height;
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  VkRect2D scissor{}; // truncates framebuffer to specified extent, can be a dynamic state
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent; // cover entire framebuffer with scissor

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;
  */
  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE; // turning this on basically discards output to framebuffer
  // fragment geometry generation
  // can be;
  // - VK_POLYGON_MODE_FILL
  // - VK_POLYGON_MODE_LINE
  // - VK_POLYGON_MODE_POINT
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f; // 1 is max lineWidth without gpu feature
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  // front face counter clockwise, as glm y-coord representation of vertices passed as uniform must be inverted
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //VK_FRONT_FACE_CLOCKWISE
  rasterizer.depthBiasEnable = VK_FALSE; // allows to alter depth values
  rasterizer.depthBiasConstantFactor = 0.0f; // optional
  rasterizer.depthBiasClamp = 0.0f; // optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // optional

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // optional
  multisampling.pSampleMask = nullptr; // optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // optional
  multisampling.alphaToOneEnable = VK_FALSE; // optional

  // depth and/or stencil buffer placeholder

  // combine fragment shader color with color already in framebuffer (single framebuffer example)
  VkPipelineColorBlendAttachmentState color_blend_attachment{}; // VkPipelineColorBlendStateCreateInfo contains GLOBAL color blending settings
  // entire setup looks like the following;
  // if (blendEnable) {
  //   final_colour.rgb = (srcColorBlendFactor * new_colour.rgb) <colorBlendOp> (dstColorBlendFactor * old_colour.rgb);
  //   final_colour.a = (srcAlphaBlendFactor * new_colour.a) <alphaBlendOp> (dstAlphaBlendFactor * old_colour.a);
  // } else {
  //     final_colour = new_colour;
  // }
  // final_colour = final_colour & colorWriteMask;
  // -------------------------------------------
  // currently imitating alpha blending;
  // final_color.rgb = new_alpha * new_color + (1 - new_alpha) * old_color;
  // final_color.a = new_alpha.a;
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

  // references the array of structures for all of the framebuffers and allows to set 
  // blend constants that can be used as blend factors in the above calculations
  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY; // optional
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f; // optional
  color_blending.blendConstants[1] = 0.0f; // optional
  color_blending.blendConstants[2] = 0.0f; // optional
  color_blending.blendConstants[3] = 0.0f; // optional

  std::vector<VkDynamicState> dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_state.pDynamicStates = dynamic_states.data();

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
  pipeline_layout_info.pushConstantRangeCount = 0;

  if(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create pipeline layout");

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = nullptr; // optional
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = pipeline_layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;

  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create graphics pipeline");

  vkDestroyShaderModule(device, vertex_shader, nullptr);
  vkDestroyShaderModule(device, fragment_shader, nullptr);
}

auto VulkanApplication::create_framebuffers(void) -> void {
  swap_chain_framebuffers.resize(swap_chain_image_views.size());

  for(std::size_t i = 0; i < swap_chain_image_views.size(); i++) {
    VkImageView attachments[] = { swap_chain_image_views[i] };

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swap_chain_extent.width;
    framebuffer_info.height = swap_chain_extent.height;
    framebuffer_info.layers = 1;

    if(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("Error - failed to create framebuffer");
  }
}

auto VulkanApplication::create_command_pool(void) -> void {
  QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

  if(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create command pool");
}

auto VulkanApplication::create_texture_image(void) -> void {
  int tex_width, tex_height, tex_channels;
  auto* pixels = stbi_load("../textures/example_a.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
  VkDeviceSize image_size = tex_width * tex_height * 4;
  
  if(!pixels)
    throw std::runtime_error("Error - failed to load texture image");

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  create_buffer(
    image_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    staging_buffer, 
    staging_buffer_memory
  );

  void* data;
  vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(image_size));
  vkUnmapMemory(device, staging_buffer_memory);

  stbi_image_free(pixels);

  create_image(
    tex_width, tex_height,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    texture_image, 
    texture_image_memory
  );

  transition_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copy_buffer_to_image(staging_buffer, texture_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
  // one last transition to prepare for shader access
  transition_image_layout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);
}

auto VulkanApplication::create_texture_image_view(void) -> void {
  texture_image_view = create_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB);
}

auto VulkanApplication::create_texture_sampler(void) -> void {
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device, &properties);

  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  // bilinear filtering; 4 closest texels combined through linear interpolation 
  // - smoother/blurred result 
  sampler_info.magFilter = VK_FILTER_LINEAR; // could also be VK_FILTER_NEAREST
  sampler_info.minFilter = VK_FILTER_LINEAR;
  // uvw instead of xyz is a convention for texture-space coordinates
  // - VK_SAMPLER_ADDRESS_MODE_REPEAT: repeat texture when going beyond the image dimensions
  // - VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: like repeat, but inverts the coordinates 
  //   to mirror the image when going beyond the dimensions
  // - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: take the color of the edge closest to the 
  //   coordinate beyond the image dimensions
  // - VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: like clamp to edge, but instead uses 
  //   edge opposite to the closest edge
  // - VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return a solid color when sampling beyond 
  //   the image dimensions
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  // limits amount of texel samples that can be used to calculate the final color
  // - lower values results in better performance, but lower quality results ** MACHINE-BASED OPTIMIZATION AVAILABLE **
  // - need to retrieve properties of physical device to see how low to make it!
  // - anisotropic filtering is actually an optional device feature, so it must be explicitly mentioned in
  //   create_logical_device function...
  sampler_info.anisotropyEnable = VK_TRUE;
  // - set maxAnisotropy to the max quality sampler value in the device property limits
  sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE; // true=[0, tex_width) & [0, text_height), false=[0, 1) & [0, 1)
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  if(vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS)
    throw std::runtime_error("failed to create texture sampler!");

  VkPhysicalDeviceFeatures device_features{};
}

auto VulkanApplication::create_vertex_buffer(void) -> void {
  VkDeviceSize buffer_size = sizeof(vulkan_vertices[0]) * vulkan_vertices.size();

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  create_buffer(
    buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    staging_buffer, staging_buffer_memory
  );

  // map buffer memory into cpu accessible memory
  void* data;
  uint32_t flags = 0;
  // access a region of the specified memory resource defined by an offset and size (VK_WHOLE_SIZE maps whole region)
  vkMapMemory(device, staging_buffer_memory, 0, buffer_size, flags, &data);
  memcpy(data, vulkan_vertices.data(),  static_cast<std::size_t>(buffer_size)); // copy vulkan_vertices into buffer memory (and therefore buffer)
  vkUnmapMemory(device, staging_buffer_memory); // unmap memory object once host access to it is no longer needed, memory is now initialized

  create_buffer(
    buffer_size, 
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    vertex_buffer, vertex_buffer_memory
  );

  // src, dest, size
  copy_buffer(staging_buffer, vertex_buffer, buffer_size);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);
}

auto VulkanApplication::create_index_buffer(void) -> void {
  VkDeviceSize buffer_size = sizeof(vulkan_indices[0]) * vulkan_indices.size();

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  create_buffer(
    buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    staging_buffer, staging_buffer_memory
  );

  void* data;
  vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
  // dest, src, size
  memcpy(data, vulkan_indices.data(), static_cast<std::size_t>(buffer_size));
  vkUnmapMemory(device, staging_buffer_memory);

  create_buffer(
    buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    index_buffer, index_buffer_memory
  );

  // src, dest, size
  copy_buffer(staging_buffer, index_buffer, buffer_size);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);
}

auto VulkanApplication::create_uniform_buffers(void) -> void {
  VkDeviceSize buffer_size = sizeof(UniformBufferObject);

  uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
  uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

  for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    create_buffer(
      buffer_size, 
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
      uniform_buffers[i],
      uniform_buffers_memory[i]);
    vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0, &uniform_buffers_mapped[i]);
  }
}

auto VulkanApplication::create_command_buffers(void) -> void {
  command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool; // buffer will be destroyed when pool is
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

  if(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to allocate command buffers");
}

auto VulkanApplication::create_sync_objects(void) -> void {
  semaphores_image_available_render.resize(MAX_FRAMES_IN_FLIGHT);
  semaphores_render_finished_present.resize(MAX_FRAMES_IN_FLIGHT);
  fences_in_flight.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  // BREAKING: fence must start in signaled state, as the draw_frame function waits for a signal to start;
  // at the start of the program there is no signal, so it needs to start in an initial 'on' state
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for(std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphores_image_available_render[i]) != VK_SUCCESS)
      throw std::runtime_error("Error - failed to create image_available_render semaphore");

    if(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphores_render_finished_present[i]) != VK_SUCCESS)
      throw std::runtime_error("Error - failed to create render_finished_present semaphore");

    if(vkCreateFence(device, &fence_info, nullptr, &fences_in_flight[i]) != VK_SUCCESS)
      throw std::runtime_error("Error - failed to create in_flight fence");
  }
}

auto VulkanApplication::create_debug_utils_messenger_ext(
  VkInstance instance, 
  const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
  const VkAllocationCallbacks* p_allocator,
  VkDebugUtilsMessengerEXT* p_debug_msnger) -> VkResult {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if(func != nullptr)
    return func(instance, p_create_info, p_allocator, p_debug_msnger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

auto VulkanApplication::destroy_debug_utils_messenger_ext(
  VkInstance instance, 
  VkDebugUtilsMessengerEXT debug_msnger, 
  const VkAllocationCallbacks* p_allocator) -> void {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if(func != nullptr)
    func(instance, debug_msnger, p_allocator);
}

auto VulkanApplication::find_queue_families(VkPhysicalDevice device) -> QueueFamilyIndices {
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  for(std::size_t i = 0; const auto& queue_family : queue_families) {
    if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_family = i;
    if(indices.is_complete()) break;
    
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if(present_support)
      indices.present_family = i;

    ++i;
  }

  return indices;
}

auto VulkanApplication::is_device_suitable(VkPhysicalDevice device) -> bool {
  auto extensions_supported = check_device_extension_support(device);
  auto indices = find_queue_families(device);

  auto swap_chain_adequate = false;
  if(extensions_supported) {
    SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
    swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
  }

  VkPhysicalDeviceFeatures supported_features;
  vkGetPhysicalDeviceFeatures(device, &supported_features);

  return indices.is_complete() && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

auto VulkanApplication::check_device_extension_support(VkPhysicalDevice device) -> bool {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

  auto required_extensions = std::set<std::string>{device_extensions.begin(), device_extensions.end()};

  for(const auto& extension : available_extensions)
    required_extensions.erase(extension.extensionName);

  return required_extensions.empty();
}

auto VulkanApplication::query_swap_chain_support(VkPhysicalDevice device) -> SwapChainSupportDetails {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  // query supported surface formats
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  if(format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
  }

  // query supported surface presentation modes
  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
  if(present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

// writes the commands want to execute into a command buffer
auto VulkanApplication::record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) -> void {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0; // optional
  begin_info.pInheritanceInfo = nullptr; // optional, secondary command buffers use

  if(vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to begin recording command buffer");

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = render_pass;
  render_pass_info.framebuffer = swap_chain_framebuffers[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = swap_chain_extent;

  VkClearValue clear_color = {{{0.2f, 0.2f, 0.2f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  // dictate the render pass and graphics pipeline objects used
  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

  VkBuffer vertex_buffers[] = {vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets); // bind vertex buffers
  vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swap_chain_extent.width);
  viewport.height = static_cast<float>(swap_chain_extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent;
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  // pipeline to use (computer or graphics), layout descriptor sets are based on, index of first desc set, #sets to bind, array to bind 
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);
  //vkCmdDraw(command_buffer, static_cast<uint32_t>(vulkan_vertices.size()), 1, 0, 0);
  // cmd_buf, number of indices, number of instances (not using instancing, so just 1)
  vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(vulkan_indices.size()), 1, 0, 0, 0);
  vkCmdEndRenderPass(command_buffer);

  if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to record command buffer");
}

auto VulkanApplication::recreate_swap_chain(void) -> void {
  // if the window is minimized (width=0 & height=0), pause until it is in foreground again 
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while(width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device); // dont touch resources still in use

  // free resources to prepare for creation
  cleanup_swap_chain();

  create_swap_chain();
  create_image_views(); // based on swap chain images
  create_render_pass(); // depends on format of swap chain images (rare to change, still good to handle)
  create_graphics_pipeline(); // viewport extent and scissor rectangle specified here
  create_framebuffers(); // directly depend on swap chain images
}

auto VulkanApplication::cleanup_swap_chain(void) -> void {
  for(auto&& framebuffer : swap_chain_framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  for(auto&& image_view : swap_chain_image_views)
    vkDestroyImageView(device, image_view, nullptr);

  vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

// "Graphics cards can offer different types of memory to allocate from. Each type of
//  memory varies in terms of allowed operations and performance characteristics.
//  Need to combine the requirements of the buffer and the application requirements
//  to find the right type of memory to use"
auto VulkanApplication::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t {
  // structure has two arrays; memoryTypes and memoryHeaps (distinct memory resources, like dedicated VRAM or RAM swap sapce)
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for(std::size_t i = 0; i < mem_properties.memoryTypeCount; ++i)
    // if a type matches the bitmask filter, then just go with that memory type
    // assert that the memory type chosen is able to support writes from the CPU (need VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    if((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;

  throw std::runtime_error("Error - failed to find suitable memory type");
}

auto VulkanApplication::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) -> void {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create vertex buffer");

  // buffer has been created, but has no memory allocated to it yet

  // 3 fields;
  // - size: size of the required amount of memory in bytes, may differ from buffer_info.size
  // - alignment: offset in bytes where the buffer begins in allocated region of memory, depends on buffer_info.usage and buffer_info.flags
  // - memoryTypeBits: bit field of the memory types that are suitable for the buffer
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

  // allocate the memory to the buffer memory region
  if(vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to allocate vertex buffer memory");

  // if the allocation was successful, bind the buffer to the memory region
  vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

auto VulkanApplication::copy_buffer(VkBuffer src_buffer, VkBuffer dest_buffer, VkDeviceSize size) -> void {
  // temporary command buffer allocation to perform transfer operation
  VkCommandBuffer command_buffer = begin_single_time_commands();

  VkBufferCopy copy_region{};
  copy_region.size = size;
  // src, dest, arr_size, arr of regions to copy
  vkCmdCopyBuffer(command_buffer, src_buffer, dest_buffer, 1, &copy_region);

  end_single_time_commands(command_buffer);
}

auto VulkanApplication::update_uniform_buffer(uint32_t current_image_index) -> void {
  static auto start_time = std::chrono::high_resolution_clock::now();

  auto current_time = std::chrono::high_resolution_clock::now();
  auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

  UniformBufferObject ubo{};

  auto trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)); // apply no transformation currently
  ubo.model = glm::rotate(trans, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  
  ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.view = glm::translate(ubo.view, main_camera.position); // POSITION OBJECT RELATIVE TO CAMERA VIEW

  // projection matrix corrects aspect ratio; rectangle appears as a square, like it should
  ubo.projection = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float) swap_chain_extent.height, 0.01f, 50.0f);
  ubo.projection[1][1] *= -1; // in OpenGL, Y-clip-coordinate is inverted, so images will render upside down

  memcpy(uniform_buffers_mapped[current_image_index], &ubo, sizeof(ubo));
}

auto VulkanApplication::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory) -> void {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = tiling;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create image");

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(device, image, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

  if(vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to allocate image memory");

  vkBindImageMemory(device, image, image_memory, 0);
}

auto VulkanApplication::begin_single_time_commands(void) -> VkCommandBuffer {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only using once, good compiler hint

  // start recording the command buffer
  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

auto VulkanApplication::end_single_time_commands(VkCommandBuffer command_buffer) -> void {
  // command buffer only contains copy command, stop recording
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue); // wait for transfer to complete, fences could allow multiple queues

  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

auto VulkanApplication::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) -> void {
  VkCommandBuffer command_buffer = begin_single_time_commands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout; // can use VK_IMAGE_LAYOUT_UNDEFINED if dont care about existing image contents
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // both must be explicitly ignored
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image; // **
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0; // TODO BELOW
  barrier.dstAccessMask = 0; // TODO BELOW

  // set access masks for pipeline stages based on layouts in the transition
  // two transitions need to handle;
  // - undefined -> transfer destination: transfer writes that don't need to wait
  // - transfer destination -> shader reading: shader reads should wait on transfer writes,
  //   specifically shader reads in fragment shader, because thats where texture is used
  // NOTES: 
  // - transfer writes must occur in the pipeline transfer stage
  // - VK_PIPELINE_STAGE_TRANSFER_BIT is not a real stage, it is a pseudo-stage where transfers happen
  // - image will be written in the same pipeline stage and subsequently read by the fragment shader
  //   (this is why specify shader reading access in the fragment shader pipeline stage)
  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else {
    throw std::invalid_argument("Error - unsupported layout transition");
  }

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  end_single_time_commands(command_buffer);
}

auto VulkanApplication::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) -> void {
  VkCommandBuffer command_buffer = begin_single_time_commands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  end_single_time_commands(command_buffer);
}

auto VulkanApplication::create_image_view(VkImage image, VkFormat format) -> VkImageView {
  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView image_view;
  if(vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create texture image view");

  return image_view;
}

auto VulkanApplication::update_glfw_delta_time(void) -> void {
  /*
  static double last_frame = 0;
  
  double current_frame = glfwGetTime();
  glfw_delta_time = current_frame - last_frame;
  last_frame = current_frame;
  */
  static auto start_time = std::chrono::high_resolution_clock::now();
  
  auto current_time = std::chrono::high_resolution_clock::now();
  glfw_delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
  start_time = current_time;
}
// ---- End of Setup/Utility ----

// ---- Rendering ----
auto VulkanApplication::draw_frame(void) -> void {
  // wait for previous frame to finish so command buffer and semaphores are available to use
  vkWaitForFences(device, 1, &fences_in_flight[current_frame], VK_TRUE, UINT64_MAX); // UINT64_MAX timeout

  uint32_t image_index;
  // aquire image from chosen device and swap chain, signal sem_image_available_render when finished
  auto result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, semaphores_image_available_render[current_frame], VK_NULL_HANDLE, &image_index);
  if(result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swap_chain(); return; // recreate swap chain, try again on next call of draw_frame
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Error - failed to acquire swap chain image");
  }

  update_uniform_buffer(current_frame);

  // reset fence to unsignaled state when done waiting, only submit when doing work
  vkResetFences(device, 1, &fences_in_flight[current_frame]);

  // ensure command buffer can be recorded by resetting
  vkResetCommandBuffer(command_buffers[current_frame], 0);
  record_command_buffer(command_buffers[current_frame], image_index);

  // queue submission and synchronization
  VkSemaphore wait_semaphores[] = {semaphores_image_available_render[current_frame]}; // sems to wait for
  VkSemaphore signal_semaphores[] = {semaphores_render_finished_present[current_frame]}; // sems to signal
  // render pass will wait for VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT (subpass in create_render_pass)
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores; // wait until done
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers[current_frame];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores; // signal when done

  // signal fence once command buffer finishes execution -> wait during next frame to finish
  if(vkQueueSubmit(graphics_queue, 1, &submit_info, fences_in_flight[current_frame]) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to submit draw command buffer");

  VkSwapchainKHR swap_chains[] = {swap_chain};

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;
  present_info.pResults = nullptr; // optional

  result = vkQueuePresentKHR(present_queue, &present_info);
  if(result == VK_ERROR_OUT_OF_DATE_KHR || 
     result == VK_SUBOPTIMAL_KHR || 
     framebuffer_resized) {
    framebuffer_resized = false;
    recreate_swap_chain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Error - failed to present swap chain image");
  }

  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
// ---- End of Rendering ----

} // end of namespace vulkan

static auto check_validation_layer_support(void) -> bool {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  // try const auto& or const auto* maybe
  for(const char* layer_name : vulkan::validation_layers) {
    auto layer_found = false;

    for(const auto& layer_properties : available_layers) {
      if(strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if(!layer_found) return false;
  }

  return true;
}

static auto message_callback_get_required_extensions(void) -> std::vector<const char*> {
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if(vulkan::enable_validation_layers)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

// first parameter is one of the following:
//
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 
// - Diagnostic message
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
// - Informational message like the creation of a resource
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 
// - Message about behavior that is not necessarily an error, but very likely a 
//   bug in your application
// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
// - Message about behavior that is invalid and may cause crashes
//
// second parameter is one of the following:
//
// VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
// - Some event has happened that is unrelated to the specification or performance
// VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: 
// - Something has happened that violates the specification or indicates a
//   possible mistake
// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
// - Potential non-optimal use of Vulkan
//
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
  VkDebugUtilsMessageTypeFlagsEXT message_type, 
  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
  void* p_user_data) {
  // uncomment for only severe messages to be displayed
  if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
  return VK_FALSE;
}

static auto populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) -> void {
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;
}

static auto choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) -> VkSurfaceFormatKHR {
  for(const auto& available_format : available_formats)
    if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return available_format;

  return available_formats[0];
}

static auto choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) -> VkPresentModeKHR {
  for(const auto& available_present_mode : available_present_modes)
    if(available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return available_present_mode;

  return VK_PRESENT_MODE_FIFO_KHR;
}


static auto choose_swap_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) -> VkExtent2D {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
      static_cast<uint32_t>(width), 
      static_cast<uint32_t>(height)
    };
    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
  }
}

static auto read_file(const std::string& file_name) -> std::vector<char> {
  auto file = std::ifstream(file_name, std::ios::ate | std::ios::binary);
  if(!file.is_open())
    throw std::runtime_error("Error - unable to open file");

  std::size_t file_size = file.tellg();
  std::vector<char> result(file_size);

  file.seekg(0);
  file.read(result.data(), file_size);
  file.close();

  return result;
}

static auto create_shader_module(VkDevice device, const std::vector<char>& code) -> VkShaderModule {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shader_module{};
  if(vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    throw std::runtime_error("Error - unable to create shader module");

  return shader_module;
}

static auto framebuffer_resize_callback(GLFWwindow* window, int width, int height) -> void {
  auto app = reinterpret_cast<vulkan::VulkanApplication*>(glfwGetWindowUserPointer(window));
  app->framebuffer_resized = true;
}
