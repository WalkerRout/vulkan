#ifndef TRIANGLE_H
#define TRIANGLE_H

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#define ENABLE_VALIDATION_LAYERS // enabled by default

// private structure forward declarations
struct QueueFamilyIndices;
struct SwapChainSupportDetails;

namespace triangle {

const std::vector<const char*> validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef ENABLE_VALIDATION_LAYERS
  const bool enable_validation_layers = true;
#else
  const bool enable_validation_layers = false;
#endif // ENABLE_VALIDATION_LAYERS

struct TriangleApplication {
  static const std::size_t WIDTH  = 800;
  static const std::size_t HEIGHT = 600;
  static const std::size_t MAX_FRAMES_IN_FLIGHT = 2;

  TriangleApplication() = default;

// ---- Main Application Pipeline ----
public:
  auto run() -> void;

private:
  auto init_window(void) -> void;
  auto init_vulkan(void) -> void;
  auto main_loop(void) -> void;
  auto cleanup(void) -> void;
// ---- End of Main Application Pipeline ----

// ---- Setup/Utility ----
public:
  auto get_window_user_ptr(void) const -> void*;

private:
  auto create_instance(void) -> void;
  auto setup_debug_messenger(void) -> void;
  auto pick_physical_device(void) -> void;
  auto create_logical_device(void) -> void;
  auto create_surface(void) -> void;
  auto create_swap_chain(void) -> void;
  auto create_image_views(void) -> void;
  auto create_render_pass(void) -> void;
  auto create_graphics_pipeline(void) -> void;
  auto create_framebuffers(void) -> void;
  auto create_command_pool(void) -> void;
  auto create_command_buffers(void) -> void;
  auto create_sync_objects(void) -> void;

  auto create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_msnger) -> VkResult;
  auto destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_msnger, const VkAllocationCallbacks* p_allocator) -> void;
  auto find_queue_families(VkPhysicalDevice device) -> QueueFamilyIndices;
  auto is_device_suitable(VkPhysicalDevice device) -> bool;
  auto check_device_extension_support(VkPhysicalDevice device) -> bool;
  auto query_swap_chain_support(VkPhysicalDevice device) -> SwapChainSupportDetails;
  auto record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) -> void;
  auto recreate_swap_chain(void) -> void;
  auto cleanup_swap_chain(void) -> void;
// ---- End of Setup/Utility ----

// ---- Rendering ----
public:
  // N/A so far
private:
  auto draw_frame(void) -> void;
// ---- End of Rendering ----

private:
  GLFWwindow* window;
  VkInstance instance;

  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR surface; // var must be in this spot -> can influence physical device selection
  
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;
  
  VkQueue graphics_queue;
  VkQueue present_queue;

  VkSwapchainKHR swap_chain;
  std::vector<VkImage> swap_chain_images;
  VkFormat swap_chain_image_format;
  VkExtent2D swap_chain_extent;

  std::vector<VkImageView> swap_chain_image_views;
  std::vector<VkFramebuffer> swap_chain_framebuffers;

  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers; // destroyed when its command pool goes out of scope

  std::vector<VkSemaphore> semaphores_image_available_render;
  std::vector<VkSemaphore> semaphores_render_finished_present;
  std::vector<VkFence> fences_in_flight;
  uint32_t current_frame = 0;
};

} // end of namespace triangle

#endif // TRIANGLE_H