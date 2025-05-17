#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

#define DEBUG true

//HELPER STRUCTS//

struct queue_family_indices 
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() 
    {
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct swapchain_support_details
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

struct swapchain_frame
{
    vk::Image image;
    vk::ImageView image_view;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer command_buffer;
};

struct swapchain_bundle
{
    vk::SwapchainKHR swapchain;
    std::vector<swapchain_frame> frames;
    vk::Format format;
    vk::Extent2D extent;
};

struct graphics_pipeline_in_bundle
{
    vk::Device device;
    std::string vertex_filepath;
    std::string fragment_filepath;
    vk::Extent2D swapchain_extent;
    vk::Format swapchain_image_format;
};

struct graphics_pipeline_out_bundle
{
    vk::PipelineLayout layout;
    vk::RenderPass renderpass;
    vk::Pipeline pipeline;
};

struct framebuffer_input_chunk
{
    vk::Device device;
    vk::RenderPass renderpass;
    vk::Extent2D swapchain_extent;
};

struct command_buffer_input_chunk
{
    vk::Device device;
    vk::CommandPool command_pool;
    std::vector<swapchain_frame> &frames;
};

std::vector<char> read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

vk::ShaderModule create_module(std::string filename, vk::Device device)
{
    std::vector<char> source_code{read_file(filename)};
    vk::ShaderModuleCreateInfo module_info{};
    module_info.flags = vk::ShaderModuleCreateFlags();
    module_info.codeSize = source_code.size();
    module_info.pCode = reinterpret_cast<const uint32_t*>(source_code.data());

    try
    {
        return device.createShaderModule(module_info);
    }
    catch(vk::SystemError err)
    {
        std::cout << "failed to create shader module for \"" << filename << "\"" << std::endl;
    }
    return nullptr;
}

//CALLBACK FUNCTIONS//
void GLFW_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

//EXTENSION SUPPORT VERIFICATION FUNCTION//

bool check_extension_support(const std::vector<const char *> &extensions)
{

    std::vector<vk::ExtensionProperties> supported_extensions = vk::enumerateInstanceExtensionProperties();

    if (DEBUG)
    {
        std::cout << "supported extensions:\n";
        for (vk::ExtensionProperties supported_extension : supported_extensions)
        {
            std::cout << '\t' << supported_extension.extensionName << '\n';
        }

        std::cout << "requested extensions:\n";
        for (const char *extension : extensions)
        {
            std::cout << '\t' << extension << '\n';
        }
    }

    bool found;

    for (const char *extension : extensions)
    {
        found = false;
        for (vk::ExtensionProperties supported_extension : supported_extensions)
        {
            if (strcmp(extension, supported_extension.extensionName) == 0)
            {
                found = true;
                if (DEBUG)
                {
                    std::cout << "extension \"" << extension << "\" is supported!\n";
                }
            }
        }
        if (!found)
        {
            if (DEBUG)
            {
                std::cout << "extension \"" << extension << "\" is not supported!\n";
            }
            return false;
        }
    }
    return true;
}

//LAYER SUPPORT VERIFICATION FUNCTION//

bool check_layer_support(const std::vector<const char *> &layers)
{

    std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties();

    if (DEBUG)
    {
        std::cout << "supported layers:\n";
        for (vk::LayerProperties supported_layer : supported_layers)
        {
            std::cout << '\t' << supported_layer.layerName << '\n';
        }

        std::cout << "requested layers:\n";
        for (const char *layer : layers)
        {
            std::cout << '\t' << layer << '\n';
        }
    }

    bool found;

    for (const char *layer : layers)
    {
        found = false;
        for (vk::LayerProperties supported_layer : supported_layers)
        {
            if (strcmp(layer, supported_layer.layerName) == 0)
            {
                found = true;
                std::cout << "layer \"" << layer << "\" is supported!\n";
            }
        }
        if (!found)
        {
            std::cout << "layer \"" << layer << "\" is not supported!\n";
            return false;
        }
    }
    return true;
}

//DEVICE HELPER FUNCTIONS//

bool check_device_support(const vk::PhysicalDevice &device, const std::vector<const char *> &requested_extensions)
{
    std::set<std::string> required_extensions(requested_extensions.begin(), requested_extensions.end());

    if (DEBUG)
    {
        std::cout << "device can support extensions:\n";
    }

    for (vk::ExtensionProperties &extension : device.enumerateDeviceExtensionProperties())
    {

        if (DEBUG)
        {
            std::cout << "\t\"" << extension.extensionName << "\"\n";
        }

        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

bool check_device_suitability(const vk::PhysicalDevice &device)
{
    if (DEBUG)
    {
        std::cout << "checking if device is suitable\n";
    }

    const std::vector<const char *> requested_extensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    if (DEBUG)
    {
        std::cout << "requested device extensions:\n";

        for (const char *extension : requested_extensions)
        {
            std::cout << "\t\"" << extension << "\"\n";
        }
    }

    if (bool extensions_supported{check_device_support(device, requested_extensions)})
    {

        if (DEBUG)
        {
            std::cout << "device can support the requested extensions!\n";
        }
    }
    else
    {

        if (DEBUG)
        {
            std::cout << "device can't support the requested extensions!\n";
        }

        return false;
    }
    return true;
}


//LOGGING FUNCTIONS//

void log_device_properties(const vk::PhysicalDevice &device)
{
    vk::PhysicalDeviceProperties device_properties{device.getProperties()};

    std::cout << "device name: " << device_properties.deviceName << '\n';

    std::cout << "device type: ";
    switch (device_properties.deviceType)
    {

    case (vk::PhysicalDeviceType::eCpu):
        std::cout << "CPU\n";
        break;

    case (vk::PhysicalDeviceType::eDiscreteGpu):
        std::cout << "discrete GPU\n";
        break;

    case (vk::PhysicalDeviceType::eIntegratedGpu):
        std::cout << "integrated GPU\n";
        break;

    case (vk::PhysicalDeviceType::eVirtualGpu):
        std::cout << "virtual GPU\n";
        break;

    default:
        std::cout << "other\n";
    }
}

std::vector<std::string> log_transform_bits(vk::SurfaceTransformFlagsKHR bits)
{
    std::vector<std::string> result;

    if (bits & vk::SurfaceTransformFlagBitsKHR::eIdentity)
    {
        result.push_back("identity");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eRotate90)
    {
        result.push_back("90 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eRotate180)
    {
        result.push_back("180 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eRotate270)
    {
        result.push_back("270 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror)
    {
        result.push_back("horizontal mirror");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90)
    {
        result.push_back("horizontal mirror, then 90 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180)
    {
        result.push_back("horizontal mirror, then 180 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270)
    {
        result.push_back("horizontal mirror, then 270 degree rotation");
    }

    if (bits & vk::SurfaceTransformFlagBitsKHR::eInherit)
    {
        result.push_back("inherited");
    }

    return result;
}

std::vector<std::string> log_alpha_composite_bits(vk::CompositeAlphaFlagsKHR bits)
{
    std::vector<std::string> result;

    if (bits & vk::CompositeAlphaFlagBitsKHR::eOpaque)
    {
        result.push_back("opaque");
    }

    if (bits & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
    {
        result.push_back("pre multiplied");
    }

    if (bits & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
    {
        result.push_back("post multiplied");
    }

    if (bits & vk::CompositeAlphaFlagBitsKHR::eInherit)
    {
        result.push_back("inherit");
    }

    return result;
}

std::vector<std::string> log_image_usage_bits(vk::ImageUsageFlags bits)
{
    std::vector<std::string> result;

    if (bits & vk::ImageUsageFlagBits::eTransferSrc)
    {
        result.push_back("transfer src");
    }
    if (bits & vk::ImageUsageFlagBits::eTransferDst)
    {
        result.push_back("transfer dst");
    }
    if (bits & vk::ImageUsageFlagBits::eSampled)
    {
        result.push_back("sampled");
    }
    if (bits & vk::ImageUsageFlagBits::eStorage)
    {
        result.push_back("storage");
    }
    if (bits & vk::ImageUsageFlagBits::eColorAttachment)
    {
        result.push_back("color attachment");
    }
    if (bits & vk::ImageUsageFlagBits::eDepthStencilAttachment)
    {
        result.push_back("depth/stencil attachment");
    }
    if (bits & vk::ImageUsageFlagBits::eTransientAttachment)
    {
        result.push_back("transient attachment");
    }
    if (bits & vk::ImageUsageFlagBits::eInputAttachment)
    {
        result.push_back("input attachment");
    }
    if (bits & vk::ImageUsageFlagBits::eFragmentDensityMapEXT)
    {
        result.push_back("fragment density map");
    }
    if (bits & vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR)
    {
        result.push_back("fragment shading rate attachment");
    }
    return result;
}

std::string log_present_mode(vk::PresentModeKHR present_mode)
{
    if (present_mode == vk::PresentModeKHR::eImmediate)
    {
        return "immediate";
    }
    if (present_mode == vk::PresentModeKHR::eMailbox)
    {
        return "mailbox";
    }
    if (present_mode == vk::PresentModeKHR::eFifo)
    {
        return "fifo";
    }
    if (present_mode == vk::PresentModeKHR::eFifoRelaxed)
    {
        return "relaxed fifo";
    }
    if (present_mode == vk::PresentModeKHR::eSharedDemandRefresh)
    {
        return "shared demand refresh";
    }
    if (present_mode == vk::PresentModeKHR::eSharedContinuousRefresh)
    {
        return "shared continuous refresh: the presentation engine and application have \
concurrent access to a single image, which is referred to as a shared presentable image. The \
presentation engine periodically updates the current image on its regular refresh cycle. The \
application is only required to make one initial presentation request, after which the \
presentation engine must update the current image without any need for further presentation \
requests. The application can indicate the image contents have been updated by making a \
presentation request, but this does not guarantee the timing of when it will be updated. \
This mode may result in visible tearing if rendering to the image is not timed correctly.";
    }
    return "none/undefined";
}

//SETUP FUNCTIONS//

GLFWwindow *setup_window()
{
    // GLFW SETUP//

    if (DEBUG)
    {
        std::cout << "GLFW setup in progress...\n";
    }

    // initialize GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("GLFW initalization failed!\n");
    }

    // check if Vulkan is supported
    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("Vulkan not supported!\n");
    }

    // pass window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // create a window
    GLFWwindow *window{glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "test window", nullptr, nullptr)};
    if (!window)
    {
        std::cerr << "GLFW window initalization failed!\n";
        glfwTerminate();
    }

    // set key callback function
    glfwSetKeyCallback(window, GLFW_key_callback);

    if (DEBUG)
    {
        std::cout << "GLFW setup complete!\n";
    }

    return window;
}

vk::Instance setup_instance()
{
    //INSTANCE SETUP//

    if (DEBUG)
    {
        std::cout << "instance setup in progress...\n";
    }

    // setup application info
    vk::ApplicationInfo app_info{
        "hello triangle",         // app name
        VK_MAKE_VERSION(1, 0, 0), // application version
        "no engine",              // engine name
        VK_MAKE_VERSION(1, 0, 0), // engine version
        VK_API_VERSION_1_0        // API version
    };

    // gather glfw extensions
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    // add extensions for mac compatability
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // extension required for debugging
    if (DEBUG)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // make sure extensions are supported by machine
    check_extension_support(extensions);

    std::vector<const char *> layers;
    if (DEBUG)
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    // make sure layers are supported by machine
    check_layer_support(layers);

    // setup instance info
    vk::InstanceCreateInfo instance_create_info{
        vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR), // flags
        &app_info,                                                                     // application info
        static_cast<uint32_t>(layers.size()),                                          // layer count
        layers.data(),                                                                 // layer names
        static_cast<uint32_t>(extensions.size()),                                      // extension size
        extensions.data()                                                              // extension names
    };

    // create the instance
    vk::Instance instance{vk::createInstance(instance_create_info)};

    if (!instance)
    {
        throw std::runtime_error("failed to create instance!\n");
    }

    if (DEBUG)
    {
        std::cout << "instance setup complete!\n";
    }

    return instance;
}

vk::DebugUtilsMessengerEXT setup_debug_messenger(const vk::Instance &instance, const vk::detail::DispatchLoaderDynamic &dldi)
{
    // DEBUG MESSENGER SETUP//

    std::cout << "debug messenger setup in progress...\n";

    // setup debug callback
    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debug_callback,
        nullptr);

    // initalize the messenger
    vk::DebugUtilsMessengerEXT debug_messenger{instance.createDebugUtilsMessengerEXT(debug_create_info, nullptr, dldi)};

    if (!debug_messenger)
    {
        throw std::runtime_error("failed to create debug messenger!\n");
    }

    //send a test message
    instance.submitDebugUtilsMessageEXT(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
        vk::DebugUtilsMessengerCallbackDataEXT()
        .setPMessage("this is a test debug message from inside the app!"),
        dldi);

    std::cout << "debug messenger setup complete!\n";

    return debug_messenger;
}

vk::PhysicalDevice setup_physical_device(const vk::Instance &instance)
{
    // PHYSICAL DEVICE SETUP//

    if (DEBUG)
    {
        std::cout << "physical device setup in progress...\n";
    }

    //initalize list of available devices
    std::vector<vk::PhysicalDevice> available_devices{instance.enumeratePhysicalDevices()};

    if (DEBUG)
    {
        std::cout << "there are " << available_devices.size() << " physical devices available on this system\n";
    }

    //check if we can use any of the available devices and return a suitable one
    for (vk::PhysicalDevice available_device : available_devices)
    {
        if (DEBUG)
        {
            log_device_properties(available_device);
        }
        if (check_device_suitability(available_device))
        {
            if (DEBUG)
            {
                std::cout << "physical device setup complete!\n";
            }
            return available_device;
        }
    }
    throw std::runtime_error("failed to find suitable device!\n");
    return nullptr;
}

queue_family_indices setup_queue_families(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    //QUEUE FAMILY SETUP//
    
    if (DEBUG)
    {
        std::cout << "queue families setup in progress...\n";
    }

    //initalize the queue family struct
    queue_family_indices indices;

    std::vector<vk::QueueFamilyProperties> queue_families{device.getQueueFamilyProperties()};

	if (DEBUG) 
    {
		std::cout << "there are " << queue_families.size() << " queue families available on the system.\n";
	}

    //find a queue family that can be used
    int i{0};
    for(vk::QueueFamilyProperties queue_family : queue_families)
    {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphics_family = i;
            if (DEBUG)
            {
                std::cout << "queue family " << i << " is suitable for graphics\n";
            }
        }

        if (device.getSurfaceSupportKHR(i, surface))
        {
            indices.present_family = i;
            if (DEBUG)
            {
                std::cout << "queue family " << i << " is suitable for presenting\n";
            }
        }

        if (indices.is_complete()) {
			break;
		}
        i++;
    }

    if (DEBUG)
    {
        std::cout << "queue family setup complete!\n";
    }

    return indices;
}

vk::Device setup_logical_device(const vk::PhysicalDevice &physical_device, const queue_family_indices &indices)
{
    //LOGICAL DEVICE SETUP//

    if (DEBUG)
    {
        std::cout << "logical device setup in progress...\n";
    }

    std::vector<uint32_t> unique_indices;
    unique_indices.push_back(indices.graphics_family.value());
    if (indices.graphics_family.value() != indices.present_family.value())
    {
        unique_indices.push_back(indices.present_family.value());
    }

    std::vector<vk::DeviceQueueCreateInfo> queue_create_info;
    float queue_priority{1.0f};

    for (uint32_t queue_family_index : unique_indices)
    {
        queue_create_info.push_back
        (
            vk::DeviceQueueCreateInfo
            {
                vk::DeviceQueueCreateFlags(),
                queue_family_index,
                1,
                &queue_priority
            }
        );
    }

    vk::PhysicalDeviceFeatures device_features{};

    //set enabled layers
    std::vector<const char *> enabled_layers;
    if (DEBUG)
    {
        enabled_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    //set enabled extensions
    std::vector<const char *> enabled_extensions;
    enabled_extensions.push_back("VK_KHR_portability_subset");
    enabled_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    //create the device
    vk::DeviceCreateInfo device_info{
        vk::DeviceCreateFlags(),
        static_cast<uint32_t>(queue_create_info.size()),
        queue_create_info.data(),
        static_cast<uint32_t>(enabled_layers.size()),
        enabled_layers.data(),
        static_cast<uint32_t>(enabled_extensions.size()),
        enabled_extensions.data(),
        &device_features};

    vk::Device device{physical_device.createDevice(device_info)};

    if (!device)
    {
        throw std::runtime_error("failed to create logical device!\n");
    }

    if (DEBUG)
    {
        std::cout << "logical device setup complete!\n";
    }
    return device;
}

std::array<vk::Queue, 2> setup_queues(const vk::Device &device, const queue_family_indices &indices)
{
    return {
        {
            device.getQueue(indices.graphics_family.value(), 0),
            device.getQueue(indices.graphics_family.value(), 0),
        }};
}

swapchain_support_details setup_swapchain_support(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
    if (DEBUG)
    {
        std::cout << "swapchain support setup in progress...\n";
    }

    swapchain_support_details support;
    support.capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);

    if (DEBUG)
    {
        std::cout << "swapchain can support the following surface capabilities:\n";

        std::cout << "\tminimum image count: " << support.capabilities.minImageCount << '\n';
        std::cout << "\tmaximum image count: " << support.capabilities.maxImageCount << '\n';    

        std::cout << "\tcurrent extent:\n";
        std::cout << "\t\twidth: " << support.capabilities.currentExtent.width << '\n';
        std::cout << "\t\theight: " << support.capabilities.currentExtent.height << '\n';

        std::cout << "\tminimum extent supported:\n";
        std::cout << "\t\twidth: " << support.capabilities.minImageExtent.width << '\n';
        std::cout << "\t\theight: " << support.capabilities.minImageExtent.height << '\n';

        std::cout << "\tmaximum extent supported:\n";
        std::cout << "\t\twidth: " << support.capabilities.maxImageExtent.width << '\n';
        std::cout << "\t\theight: " << support.capabilities.maxImageExtent.height << '\n';   
        
        std::cout << "\tmaximum image array layers: " << support.capabilities.maxImageArrayLayers << '\n';

        std::vector<std::string> string_list;
        
        std::cout << "\tsupported transfroms:\n";
        string_list = log_transform_bits(support.capabilities.supportedTransforms);

        for (std::string line : string_list)
        {
            std::cout << "\t\t" << line << '\n';
        }

        std::cout << "\tcurrent transform:\n";
        string_list = log_transform_bits(support.capabilities.currentTransform);

        for (std::string line : string_list)
        {
            std::cout << "\t\t" << line << '\n';
        }

        std::cout << "\tsupported alpha operations:\n";
        string_list = log_alpha_composite_bits(support.capabilities.supportedCompositeAlpha);

        for (std::string line : string_list)
        {
            std::cout << "\t\t" << line << '\n';
        }

        std::cout << "\tsupported image usage:\n";
        string_list = log_image_usage_bits(support.capabilities.supportedUsageFlags);
        for (std::string line : string_list)
        {
            std::cout << "\t\t" << line << '\n';
        }

        support.formats = physical_device.getSurfaceFormatsKHR(surface);
        std::cout << "\tsupported formats:\n";
        if (DEBUG)
        {
            for (vk::SurfaceFormatKHR supported_format : support.formats)
            {
                std::cout << "\t\tsupported pixel format: " << vk::to_string(supported_format.format) << '\n';
                std::cout << "\t\tsupported color space: " << vk::to_string(supported_format.colorSpace) << '\n';
            }
        }

        std::cout << "\tsupported present modes:\n";
        support.present_modes = physical_device.getSurfacePresentModesKHR(surface);
        if (DEBUG)
        {
            for (vk::PresentModeKHR presentMode : support.present_modes)
            {
                std::cout << "\t\t" << log_present_mode(presentMode) << '\n';
            }
        }
    }

    if (DEBUG)
    {
        std::cout << "swapchain support setup complete!\n";
    }

    return support;
}

vk::SurfaceFormatKHR choose_swapchain_surface_format(std::vector<vk::SurfaceFormatKHR> formats)
{
    for (vk::SurfaceFormatKHR format : formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return format;
            }
    }
    return formats[0];
}

vk::PresentModeKHR choose_swapchain_surface_present_mode(std::vector<vk::PresentModeKHR> present_modes)
{
    for (vk::PresentModeKHR present_mode : present_modes)
    {
        if (present_mode == vk::PresentModeKHR::eFifo)
        {
            return present_mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D choose_swapchain_extent(uint32_t width, uint32_t height, vk::SurfaceCapabilitiesKHR capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        vk::Extent2D extent{width, height};

        extent.width = std::min(capabilities.maxImageExtent.width, std::max(capabilities.minImageExtent.width, width));
        extent.height = std::min(capabilities.maxImageExtent.height, std::max(capabilities.minImageExtent.height, height));

        return extent;
    }
}

swapchain_bundle setup_swapchain(vk::Device device, vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, queue_family_indices indices, int width, int height)
{
    if (DEBUG)
    {
        std::cout << "swapchain setup in progress...\n";
    }

    swapchain_support_details support{setup_swapchain_support(physical_device, surface)};
   
    vk::SurfaceFormatKHR format{choose_swapchain_surface_format(support.formats)};
    
    vk::PresentModeKHR present_mode{choose_swapchain_surface_present_mode(support.present_modes)};
    
    vk::Extent2D extent{choose_swapchain_extent(width, height, support.capabilities)};

    uint32_t image_count = std::min(support.capabilities.maxImageCount, support.capabilities.minImageCount + 1);

    vk::SwapchainCreateInfoKHR sc_create_info
    {
        vk::SwapchainCreateFlagsKHR(), 
        surface, 
        image_count, 
        format.format, 
        format.colorSpace, 
        extent, 
        1, 
        vk::ImageUsageFlagBits::eColorAttachment
    };

    uint32_t q_f_indices[2]{indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family.value() != indices.present_family.value())
    {
        sc_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        sc_create_info.queueFamilyIndexCount = 2;
        sc_create_info.pQueueFamilyIndices = q_f_indices;
    }
    else
    {
        sc_create_info.imageSharingMode = vk::SharingMode::eExclusive;
    }
    sc_create_info.preTransform = support.capabilities.currentTransform;
    sc_create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    sc_create_info.presentMode = present_mode;
    sc_create_info.clipped = VK_TRUE;

    sc_create_info.oldSwapchain = vk::SwapchainKHR(nullptr);

    swapchain_bundle bundle{};
    try
    {
        bundle.swapchain = device.createSwapchainKHR(sc_create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("failed to create swapchain!");
    }

    std::vector<vk::Image> images = device.getSwapchainImagesKHR(bundle.swapchain);
    bundle.frames.resize(images.size());

    for(size_t i{}; i < images.size(); i++)
    {
        vk::ImageViewCreateInfo iv_create_info{};
        iv_create_info.image = images[i];
        iv_create_info.viewType = vk::ImageViewType::e2D;
        iv_create_info.components.r = vk::ComponentSwizzle::eIdentity;
        iv_create_info.components.g = vk::ComponentSwizzle::eIdentity;
        iv_create_info.components.b = vk::ComponentSwizzle::eIdentity;
        iv_create_info.components.a = vk::ComponentSwizzle::eIdentity;
        iv_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        iv_create_info.subresourceRange.baseMipLevel = 0;
        iv_create_info.subresourceRange.levelCount = 1; 
        iv_create_info.subresourceRange.baseArrayLayer = 0;
        iv_create_info.subresourceRange.layerCount = 1;
        iv_create_info.format = format.format;

        bundle.frames[i].image = images[i];
        bundle.frames[i].image_view = device.createImageView(iv_create_info);
    }

    bundle.format = format.format;
    bundle.extent = extent;



    if (DEBUG)
    {
        std::cout << "swapchain setup complete!\n";
    }
    return bundle;
}

vk::PipelineLayout make_pipeline_layout(vk::Device device)
{
    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.flags = vk::PipelineLayoutCreateFlags();
    layout_info.setLayoutCount = 0;
    layout_info.pushConstantRangeCount = 0;

    try
    {
        return device.createPipelineLayout(layout_info);
    }
    catch (vk::SystemError err)
    {
        std::cout << "failed to create pipeline layout!" << std::endl;
    }

    return nullptr;
}

vk::RenderPass make_renderpass(vk::Device device, vk::Format swapchain_image_format)
{
    vk::AttachmentDescription color_attachment{};
    color_attachment.flags = vk::AttachmentDescriptionFlags();
    color_attachment.format = swapchain_image_format;
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.flags = vk::SubpassDescriptionFlags();
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    vk::RenderPassCreateInfo renderpass_info{};
    renderpass_info.flags = vk::RenderPassCreateFlags();
    renderpass_info.attachmentCount = 1;
    renderpass_info.pAttachments = &color_attachment;
    renderpass_info.subpassCount = 1;
    renderpass_info.pSubpasses = &subpass;
    try
    {
        return device.createRenderPass(renderpass_info);
    }
    catch (vk::SystemError err)
    {
        std::cout << "failed to create renderpass!" << std::endl;
    }
    return nullptr;
}

graphics_pipeline_out_bundle make_graphics_pipeline(graphics_pipeline_in_bundle specification)
{
    vk::GraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.flags = vk::PipelineCreateFlags();

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

    // vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.flags = vk::PipelineVertexInputStateCreateFlags();
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    pipeline_info.pVertexInputState = &vertex_input_info;

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.flags = vk::PipelineInputAssemblyStateCreateFlags();
    input_assembly_info.topology = vk::PrimitiveTopology::eTriangleList;
    pipeline_info.pInputAssemblyState = &input_assembly_info;

    // vertex shader
    if (DEBUG)
    {
        std::cout << "creating vertex shader module..." << std::endl;
    }

    vk::ShaderModule vertex_shader = create_module(specification.vertex_filepath, specification.device);
    vk::PipelineShaderStageCreateInfo vertex_shader_info{};
    vertex_shader_info.flags = vk::PipelineShaderStageCreateFlags();
    vertex_shader_info.stage = vk::ShaderStageFlagBits::eVertex;
    vertex_shader_info.module = vertex_shader;
    vertex_shader_info.pName = "main";
    shader_stages.push_back(vertex_shader_info);

    if (DEBUG)
    {
        std::cout << "vertex shader module created!" << std::endl;
    }

    // viewport and scissor
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = specification.swapchain_extent.width;
    viewport.height = specification.swapchain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vk::Rect2D scissor{};
    scissor.offset.x = 0.0f;
    scissor.offset.y = 0.0f;
    scissor.extent = specification.swapchain_extent;
    vk::PipelineViewportStateCreateInfo viewport_state{};
    viewport_state.flags = vk::PipelineViewportStateCreateFlags();
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;
    pipeline_info.pViewportState = &viewport_state;

    // rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.flags = vk::PipelineRasterizationStateCreateFlags();
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    pipeline_info.pRasterizationState = &rasterizer;

    // fragment shader
    if (DEBUG)
    {
        std::cout << "creating fragment shader module..." << std::endl;
    }

    vk::ShaderModule fragment_shader = create_module(specification.fragment_filepath, specification.device);
    vk::PipelineShaderStageCreateInfo fragment_shader_info{};
    fragment_shader_info.flags = vk::PipelineShaderStageCreateFlags();
    fragment_shader_info.stage = vk::ShaderStageFlagBits::eFragment;
    fragment_shader_info.module = fragment_shader;
    fragment_shader_info.pName = "main";
    shader_stages.push_back(fragment_shader_info);

    if (DEBUG)
    {
        std::cout << "fragment shader module created!" << std::endl;
    }
    pipeline_info.stageCount = shader_stages.size();
    pipeline_info.pStages = shader_stages.data();

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.flags = vk::PipelineMultisampleStateCreateFlags();
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    pipeline_info.pMultisampleState = &multisampling;

    // color blend
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = VK_FALSE;
    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.flags = vk::PipelineColorBlendStateCreateFlags();
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eCopy;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;
    pipeline_info.pColorBlendState = &color_blending;

    // pipeline layout
    if (DEBUG)
    {
        std::cout << "creating pipeline layout..." << std::endl;
    }

    vk::PipelineLayout layout{make_pipeline_layout(specification.device)};
    pipeline_info.layout = layout;

    if (DEBUG)
    {
        std::cout << "pipeline layout created!" << std::endl;
    }

    // renderpass
    if (DEBUG)
    {
        std::cout << "creating renderpass..." << std::endl;
    }

    vk::RenderPass renderpass{make_renderpass(specification.device, specification.swapchain_image_format)};
    pipeline_info.renderPass = renderpass;
    pipeline_info.subpass = 0;

    if (DEBUG)
    {
        std::cout << "renderpass created!" << std::endl;
    }

    // extra stuff
    pipeline_info.basePipelineHandle = nullptr;

    // make the pipeline
    if (DEBUG)
    {
        std::cout << "creating graphics pipeline..." << std::endl;
    }

    vk::Pipeline graphics_pipeline{specification.device.createGraphicsPipeline(nullptr, pipeline_info).value};

    graphics_pipeline_out_bundle output{};
    output.layout = layout;
    output.renderpass = renderpass;
    output.pipeline = graphics_pipeline;

    if (DEBUG)
    {
        std::cout << "graphics pipeline created!" << std::endl;
    }

    specification.device.destroyShaderModule(vertex_shader);
    specification.device.destroyShaderModule(fragment_shader);

    return output;
}

void make_framebuffers(framebuffer_input_chunk input_chunk, std::vector<swapchain_frame> &frames)
{
    for (int i{0}; i < frames.size(); i++)
    {
        std::vector<vk::ImageView> attachments{frames[i].image_view};
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.flags = vk::FramebufferCreateFlags();
        framebuffer_info.renderPass = input_chunk.renderpass;
        framebuffer_info.attachmentCount = attachments.size();
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = input_chunk.swapchain_extent.width;
        framebuffer_info.height = input_chunk.swapchain_extent.height;
        framebuffer_info.layers = 1;

        frames[i].framebuffer = input_chunk.device.createFramebuffer(framebuffer_info);

        if (DEBUG)
        {
            std::cout << "Created framebuffer for frame " << i << std::endl;
        }
    }
}

vk::CommandPool make_command_pool(vk::Device device, vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, queue_family_indices indices)
{
    vk::CommandPoolCreateInfo pool_info;
    pool_info.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = indices.graphics_family.value();

    return device.createCommandPool(pool_info);
}

vk::CommandBuffer make_command_buffers(command_buffer_input_chunk input_chunk)
{
    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandPool = input_chunk.command_pool;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandBufferCount = 1;

    for (int i{0}; i < input_chunk.frames.size(); i++)
    {
        input_chunk.frames[i].command_buffer = input_chunk.device.allocateCommandBuffers(alloc_info)[0];
    }

    vk::CommandBuffer command_buffer{input_chunk.device.allocateCommandBuffers(alloc_info)[0]};

    return command_buffer;
}

vk::Semaphore make_semaphore(vk::Device device)
{
    vk::SemaphoreCreateInfo semaphore_info{};
    semaphore_info.flags = vk::SemaphoreCreateFlags();

    return device.createSemaphore(semaphore_info);
}

vk::Fence make_fence(vk::Device device)
{
    vk::FenceCreateInfo fence_info{};
    fence_info.flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled;

    return device.createFence(fence_info);
}
/*
void record_draw_commands(vk::CommandBuffer command_buffer, vk::RenderPass renderpass, std::vector<swapchain_frame> swapchain_frames, vk::Extent2D swapchain_extent, vk::Pipeline pipeline, uint32_t image_index)
{
    vk::CommandBufferBeginInfo begin_info{};
    command_buffer.begin(begin_info);

    vk::RenderPassBeginInfo renderpass_info{};
    renderpass_info.renderPass = renderpass;
    renderpass_info.framebuffer = swapchain_frames[image_index].framebuffer;
    renderpass_info.renderArea.offset.x = 0;
    renderpass_info.renderArea.offset.y = 0;
    renderpass_info.renderArea.extent = swapchain_extent;
    vk::ClearValue clear_color{std::array<float,4>{1.0f, 0.5f, 0.25f, 1.0f}};
    renderpass_info.clearValueCount = 1;
    renderpass_info.pClearValues = &clear_color;

    command_buffer.beginRenderPass(&renderpass_info, vk::SubpassContents::eInline);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    command_buffer.draw(3, 1, 0, 0);

    command_buffer.endRenderPass();
}

void render(vk::Device device, vk::Fence in_flight_fence, vk::SwapchainKHR swapchain, vk::Semaphore image_available, std::vector<swapchain_frame> swapchain_frames)
{
    device.waitForFences(1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    device.resetFences(1, &in_flight_fence);

    uint32_t image_index{device.acquireNextImageKHR(swapchain, UINT64_MAX, image_available, nullptr).value};

    vk::CommandBuffer command_buffer = swapchain_frames[image_index].command_buffer;

    command_buffer.reset();

    record_draw_commands(command_buffer, );
}

*/

//MAIN//
int main()
{
    //setup GLFW window
    GLFWwindow *window{setup_window()};

    //setup Vulkan instance
    vk::Instance instance{setup_instance()};

    //setup debug messanger
    vk::detail::DispatchLoaderDynamic dldi{instance, vkGetInstanceProcAddr};
    vk::DebugUtilsMessengerEXT debug_messenger{};
    
    if (DEBUG)
    {
        debug_messenger = setup_debug_messenger(instance, dldi);
    }

    // setup surface
    VkSurfaceKHR c_style_surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &c_style_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create surface!\n");
    }

    vk::SurfaceKHR surface{c_style_surface};

    //setup device
    vk::PhysicalDevice physical_device{setup_physical_device(instance)};
    queue_family_indices indices{setup_queue_families(physical_device, surface)};
    vk::Device device{setup_logical_device(physical_device, indices)};
    std::array<vk::Queue, 2> queues{setup_queues(device, indices)};
    vk::Queue graphics_queue = queues[0];
    vk::Queue present_queue = queues[1];

    //setup swapchain
    swapchain_bundle bundle{setup_swapchain(device, physical_device, surface, indices, WINDOW_WIDTH, WINDOW_HEIGHT)};
    vk::SwapchainKHR swapchain{bundle.swapchain};
    std::vector<swapchain_frame> swapchain_frames{bundle.frames};
    vk::Format swapchain_format{bundle.format};
    vk::Extent2D swapchain_extent{bundle.extent};

    //setup pipeline
    graphics_pipeline_in_bundle specification{};
    specification.device = device;
    specification.vertex_filepath = "shaders/vert.spv";
    specification.fragment_filepath = "shaders/frag.spv";
    specification.swapchain_extent = swapchain_extent;
    specification.swapchain_image_format = swapchain_format;

    graphics_pipeline_out_bundle output{make_graphics_pipeline(specification)};
    vk::PipelineLayout layout{output.layout};
    vk::RenderPass renderpass{output.renderpass};
    vk::Pipeline pipeline{output.pipeline};

    //framebuffers
    framebuffer_input_chunk framebuffer_input;
    framebuffer_input.device = device;
    framebuffer_input.renderpass = renderpass;
    framebuffer_input.swapchain_extent = swapchain_extent;
    make_framebuffers(framebuffer_input, swapchain_frames);

    //command
    vk::CommandPool command_pool{make_command_pool(device, physical_device, surface, indices)};
    command_buffer_input_chunk command_buffer_input{device, command_pool, swapchain_frames};
    vk::CommandBuffer main_command_buffer{make_command_buffers(command_buffer_input)};

    //sych
    vk::Fence in_flight_fence{make_fence(device)};
    vk::Semaphore image_available{make_semaphore(device)};
    vk::Semaphore render_finished{make_semaphore(device)};

    //RUNTIME LOOP//

    if (DEBUG)
    {
        std::cout << "starting runtime loop...\n";
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    if (DEBUG)
    {
        std::cout << "runtime loop ended!\n";
    }

    //CLEANUP//

    if (DEBUG)
    {
        std::cout << "cleanup in progress...\n";
    }

    if (DEBUG)
    {
        instance.destroyDebugUtilsMessengerEXT(debug_messenger, nullptr, dldi);
    }

    device.destroyFence(in_flight_fence);
    device.destroySemaphore(image_available);
    device.destroySemaphore(render_finished);

    device.destroyCommandPool(command_pool);

    device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(layout);
    device.destroyRenderPass(renderpass);

    //vulkan cleanup
    for (swapchain_frame frame : swapchain_frames)
    {
        device.destroyImageView(frame.image_view);
        device.destroyFramebuffer(frame.framebuffer);
    }

    device.destroySwapchainKHR(swapchain);
    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroy();

    //glfw clean up
    glfwDestroyWindow(window);
    glfwTerminate();

    if (DEBUG)
    {
        std::cout << "cleanup complete!\n";
    }
    
    return 0;
}