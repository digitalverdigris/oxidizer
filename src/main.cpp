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

GLFWwindow* setup_window()
{
    //GLFW SETUP//

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
                std::cout << "extension \"" << extension << "\" is supported!\n";
            }
        }
        if (!found)
        {
            std::cout << "extension \"" << extension << "\" is not supported!\n";
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
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

    // add extension for mac compatability
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

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

    if (DEBUG)
    {
        std::cout << "debug messenger setup in progress...\n";
    }

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
    if(DEBUG)
    {
        instance.submitDebugUtilsMessageEXT(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral,
            vk::DebugUtilsMessengerCallbackDataEXT()
                .setPMessage("this is a test debug message from inside the app!"),
            dldi);
    }

    if (DEBUG)
    {
        std::cout << "debug messenger setup complete!\n";
    }

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

queue_family_indices setup_queue_families(const vk::PhysicalDevice &device)
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
            indices.present_family = i;
        }

        if (DEBUG) 
        {
		    std::cout << "queue family " << i << " is suitable for graphics and presenting.\n";
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

int main()
{
    //setup GLFW window
    GLFWwindow *window{setup_window()};

    //setup Vulkan instance
    vk::Instance instance{setup_instance()};

    //setup debug messanger
    
    vk::detail::DispatchLoaderDynamic dldi{instance, vkGetInstanceProcAddr};
    vk::DebugUtilsMessengerEXT debug_messenger{setup_debug_messenger(instance, dldi)};

    //setup device
    vk::PhysicalDevice device{setup_physical_device(instance)};

    //setup queue_familes
    queue_family_indices queue_family{setup_queue_families(device)};

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

    instance.destroyDebugUtilsMessengerEXT(debug_messenger, nullptr, dldi);

    //vulkan instance cleanup
    instance.destroy();

    //glfw window and session clean up
    glfwDestroyWindow(window);
    glfwTerminate();

    if (DEBUG)
    {
        std::cout << "cleanup complete!\n";
    }
    
    return 0;
}