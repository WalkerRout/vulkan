#ifndef VULKAN_H
#define VULKAN_H

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

namespace vulkan {

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

struct VulkanApplication {
  static const std::size_t WIDTH  = 800;
  static const std::size_t HEIGHT = 600;
  static const std::size_t MAX_FRAMES_IN_FLIGHT = 2;

  VulkanApplication() = default;

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
  auto create_descriptor_set_layout(void) -> void;
  auto create_descriptor_pool(void) -> void;
  auto create_descriptor_sets(void) -> void;
  auto create_graphics_pipeline(void) -> void;
  auto create_framebuffers(void) -> void;
  auto create_command_pool(void) -> void;
  auto create_texture_image(void) -> void;
  auto create_texture_image_view(void) -> void;
  auto create_vertex_buffer(void) -> void;
  auto create_index_buffer(void) -> void;
  auto create_uniform_buffers(void) -> void;
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
  auto find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) -> uint32_t;
  auto create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) -> void;
  auto copy_buffer(VkBuffer src_buffer, VkBuffer dest_buffer, VkDeviceSize size) -> void;
  auto update_uniform_buffer(uint32_t current_image_index) -> void;
  auto create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory) -> void;
  auto begin_single_time_commands(void) -> VkCommandBuffer;
  auto end_single_time_commands(VkCommandBuffer command_buffer) -> void;
  auto transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) -> void;
  auto copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) -> void;
  auto create_image_view(VkImage image, VkFormat format) -> VkImageView;
// ---- End of Setup/Utility ----

// ---- Rendering ----
public:
  // N/A so far
private:
  auto draw_frame(void) -> void;
// ---- End of Rendering ----

// ---- Class Members ----
public:
  bool framebuffer_resized{false};

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
  uint32_t current_frame{0};

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;

  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout descriptor_set_layout;
  std::vector<VkDescriptorSet> descriptor_sets; // one for each frame in flight

  // as many uniform buffers as frames in flight
  std::vector<VkBuffer> uniform_buffers;
  std::vector<VkDeviceMemory> uniform_buffers_memory;
  std::vector<void*> uniform_buffers_mapped;

  VkImage texture_image;
  VkDeviceMemory texture_image_memory;
  VkImageView texture_image_view;
// ---- End of Class Members ----
};

} // end of namespace vulkan

#endif // VULKAN_H