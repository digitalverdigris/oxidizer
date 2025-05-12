#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

#define DEBUG true

GLFWwindow *window = nullptr;

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

bool check_extensions(std::vector<const char *> &extensions)
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

bool check_layers(std::vector<const char *> &layers)
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

int main()
{
    //GLFW SETUP//

    if (DEBUG)
    {
        std::cout << "GLFW setup in progress...\n";
    }

    //initialize GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("GLFW initalization failed!\n");
    }

    //check if Vulkan is supported
    if (!glfwVulkanSupported())
    {
        throw std::runtime_error("Vulkan not supported!\n");
    }

    //pass window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    //create a window
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Test Window", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "GLFW window initalization failed!\n";
        glfwTerminate();
        return -1;
    }

    //set key callback function
    glfwSetKeyCallback(window, GLFW_key_callback);

    if (DEBUG)
    {
        std::cout << "GLFW setup complete!\n";
    }

    //INSTANCE SETUP//

    if (DEBUG)
    {
        std::cout << "instance setup in progress...\n";
    }

    //setup application info
    vk::ApplicationInfo app_info
    {
        "hello triangle", //app name
        VK_MAKE_VERSION(1, 0, 0), //application version
        "no engine", //engine name
        VK_MAKE_VERSION(1, 0, 0), //engine version
        VK_API_VERSION_1_0 //API version
    };

    //gather glfw extensions
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    //add extension for mac compatability
    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    //extension required for debugging
    if (DEBUG)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //make sure extensions are supported by machine
    check_extensions(extensions);

    std::vector<const char *> layers;
    if (DEBUG)
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    // make sure layers are supported by machine
    check_layers(layers);

    //setup instance info
    vk::InstanceCreateInfo instance_create_info
    {
        vk::InstanceCreateFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR), //flags
        &app_info, //application info
        static_cast<uint32_t>(layers.size()), //layer count
        layers.data(), //layer names
        static_cast<uint32_t>(extensions.size()), //extension size
        extensions.data() //extension names
    };

    //create the instance
    vk::Instance instance{vk::createInstance(instance_create_info)};

    if (!instance)
    {
        throw std::runtime_error("failed to create instance!\n");
    }

    if (DEBUG)
    {
        std::cout << "instance setup complete!\n";
    }

    //setup debug callback
    vk::detail::DispatchLoaderDynamic dldi{instance, vkGetInstanceProcAddr};

    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debug_callback,
        nullptr);

    vk::DebugUtilsMessengerEXT debug_messenger{instance.createDebugUtilsMessengerEXT(debug_create_info, nullptr, dldi)};

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
