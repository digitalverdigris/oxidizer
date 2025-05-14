#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <set>

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

struct swapchain_bundle
{
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    vk::Format format;
    vk::Extent2D extent;
};

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

std::array<vk::Queue,2> setup_queues(const vk::Device &logical_device, const queue_family_indices &indices)
{
    return 
    {
        {
        logical_device.getQueue(indices.graphics_family.value(), 0),
        logical_device.getQueue(indices.graphics_family.value(), 0),
        }
    };
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
        extent.height = std::min(capabilities.maxImageExtent.height, std::max(capabilities.minImageExtent.width, height));

        return extent;
    }
}

swapchain_bundle setup_swapchain(vk::Device logical_device, vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, queue_family_indices indices, int width, int height)
{
    if (DEBUG)
    {
        std::cout << "swapchain setup in progress...\n";
    }

    swapchain_support_details support{setup_swapchain_support(physical_device, surface)};
   
    vk::SurfaceFormatKHR format{choose_swapchain_surface_format(support.formats)};
    
    vk::PresentModeKHR present_mode{choose_swapchain_surface_present_mode(support.present_modes)};
    
    vk::Extent2D extent{choose_swapchain_extent(width, height, support.capabilities)};

    uint32_t image_count = std::min(support.capabilities.maxImageCount, support.capabilities.minImageCount);

    vk::SwapchainCreateInfoKHR create_info
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
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = q_f_indices;
    }
    else
    {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
    }
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    create_info.oldSwapchain = vk::SwapchainKHR(nullptr);

    swapchain_bundle bundle{};
    try
    {
        bundle.swapchain = logical_device.createSwapchainKHR(create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("failed to create swapchain!");
    }
    
    bundle.images = logical_device.getSwapchainImagesKHR(bundle.swapchain);
    bundle.format = format.format;
    bundle.extent = extent;

    if (DEBUG)
    {
        std::cout << "swapchain setup complete!\n";
    }

    return bundle;
}

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
    vk::Device logical_device{setup_logical_device(physical_device, indices)};
    std::array<vk::Queue, 2> queues{setup_queues(logical_device, indices)};
    swapchain_bundle bundle{setup_swapchain(logical_device, physical_device, surface, indices, WINDOW_WIDTH, WINDOW_HEIGHT)};
    vk::SwapchainKHR swapchain{bundle.swapchain};

    //RUNTIME LOOP//

    if (DEBUG)
    {
        std::cout << "starting runtime loop...\n";
    }

    /*
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
    */

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

    //vulkan cleanup
    logical_device.destroySwapchainKHR(swapchain);
    logical_device.destroy();
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