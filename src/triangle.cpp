
#include <iostream>
#include <stdexcept>
#include <optional>
#include <cstdlib>
#include <cstring>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include "triangle.h"

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

namespace triangle {

auto TriangleApplication::run(void) -> void {
  init_window();
  init_vulkan();
  main_loop();
  cleanup();
}

auto TriangleApplication::init_window(void) -> void {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "TriangleApplication", nullptr, nullptr);
  if(window == nullptr)
    throw std::runtime_error("Error - (window) is nullptr");

  // optional user pointer for the window, currently points to its owning object
  glfwSetWindowUserPointer(window, this);
}

auto TriangleApplication::init_vulkan(void) -> void {
  create_instance();
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  create_logical_device();
  create_swap_chain();
  create_image_views();
  create_render_pass();
  create_graphics_pipeline();
}

auto TriangleApplication::main_loop(void) -> void {
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}

auto TriangleApplication::cleanup(void) -> void {
  // vulkan cleanup
  vkDestroyPipeline(device, graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  vkDestroyRenderPass(device, render_pass, nullptr);

  for(auto&& image_view : swap_chain_image_views) {
    vkDestroyImageView(device, image_view, nullptr);
  }

  vkDestroySwapchainKHR(device, swap_chain, nullptr);
  vkDestroyDevice(device, nullptr);

  if(enable_validation_layers)
    destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  // glfw cleanup
  glfwDestroyWindow(window);
  glfwTerminate();
}
// End of Main Application Pipeline

// Start of Utility
auto TriangleApplication::get_window_user_ptr(void) const -> void* {
  return glfwGetWindowUserPointer(window);
}

auto TriangleApplication::create_instance(void) -> void {
  if(enable_validation_layers && !check_validation_layer_support())
    throw std::runtime_error("Error - validation layers requested, but unavailable");

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Triangle Application";
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

auto TriangleApplication::setup_debug_messenger(void) -> void {
  if(!enable_validation_layers) return;

  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  populate_debug_messenger_create_info(create_info);

  if(create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
    throw std::runtime_error("failed to set up debug messenger!");
}

auto TriangleApplication::pick_physical_device(void) -> void {
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

auto TriangleApplication::create_logical_device(void) -> void {
  QueueFamilyIndices indices = find_queue_families(physical_device);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  auto unique_queue_families = std::set<uint32_t>{indices.graphics_family.value(), indices.present_family.value()};

  auto queue_priority = 1.f;
  for(auto queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};

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

auto TriangleApplication::create_surface(void) -> void {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    throw std::runtime_error("Error - failed to create window surface");
}

auto TriangleApplication::create_swap_chain(void) -> void {
  auto swap_chain_support = query_swap_chain_support(physical_device);

  VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
  VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
  VkExtent2D extent = choose_swap_extent(window, swap_chain_support.capabilities);

  // specifying minimum may cause interal delays, so request min+1
  auto image_count = swap_chain_support.capabilities.minImageCount + 1;

  if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
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

auto TriangleApplication::create_image_views(void) -> void {
  auto image_count = swap_chain_images.size();
  swap_chain_image_views.resize(image_count);

  for(int i = 0; i < image_count; ++i) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain_image_format;

    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to create image views!");
  }
}

auto TriangleApplication::create_render_pass(void) -> void {
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

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &colour_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;

  if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
    throw std::runtime_error("failed to create render pass!");
}

auto TriangleApplication::create_graphics_pipeline(void) -> void {
  // src/triangle.cpp -> shaders/vert.spv & shaders/frag.spv
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

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 0;
  vertex_input_info.pVertexBindingDescriptions = nullptr; // optional
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions = nullptr; // optional

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
  rasterizer.lineWidth = 1.f; // 1 is max lineWidth without gpu feature
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE; // allows to alter depth values
  rasterizer.depthBiasConstantFactor = 0.f; // optional
  rasterizer.depthBiasClamp = 0.f; // optional
  rasterizer.depthBiasSlopeFactor = 0.f; // optional

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.f; // optional
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
  pipeline_layout_info.setLayoutCount = 0;
  pipeline_layout_info.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
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

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline!");

  vkDestroyShaderModule(device, vertex_shader, nullptr);
  vkDestroyShaderModule(device, fragment_shader, nullptr);
}

auto TriangleApplication::create_framebuffers(void) -> void {
  
}

auto TriangleApplication::create_debug_utils_messenger_ext(
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

auto TriangleApplication::destroy_debug_utils_messenger_ext(
  VkInstance instance, 
  VkDebugUtilsMessengerEXT debug_msnger, 
  const VkAllocationCallbacks* p_allocator) -> void {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(instance, debug_msnger, p_allocator);
}

auto TriangleApplication::find_queue_families(VkPhysicalDevice device) -> QueueFamilyIndices {
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  for (int i = 0; const auto& queue_family : queue_families) {
    if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_family = i;
    if (indices.is_complete()) break;
    
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if(present_support)
      indices.present_family = i;

    ++i;
  }

  return indices;
}

auto TriangleApplication::is_device_suitable(VkPhysicalDevice device) -> bool {
  auto extensions_supported = check_device_extension_support(device);
  auto indices = find_queue_families(device);

  auto swap_chain_adequate = false;
  if(extensions_supported) {
    SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
    swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
  }

  return indices.is_complete() && extensions_supported && swap_chain_adequate;
}

auto TriangleApplication::check_device_extension_support(VkPhysicalDevice device) -> bool {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

  auto required_extensions = std::set<std::string>{device_extensions.begin(), device_extensions.end()};

  for(const auto& extension : available_extensions)
    required_extensions.erase(extension.extensionName);

  return required_extensions.empty();
}

auto TriangleApplication::query_swap_chain_support(VkPhysicalDevice device) -> SwapChainSupportDetails {
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
// End of Utility

} // end of namespace triangle

static auto check_validation_layer_support(void) -> bool {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  // try const auto& or const auto* maybe
  for(const char* layer_name : triangle::validation_layers) {
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

  if(triangle::enable_validation_layers)
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
  //if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
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
    throw std::runtime_error("Error - unable to open shader SPIR-V bytecode file");

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