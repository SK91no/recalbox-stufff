import os
from configgen.utils.architecture import Architecture

class Vulkan:

    @staticmethod
    def vulkanAvailable():
        # Remove the pi5 condition when vulkan available
        return os.path.exists("/usr/lib/libvulkan.so")
