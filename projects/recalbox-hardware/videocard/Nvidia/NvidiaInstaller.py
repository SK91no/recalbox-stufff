#!/usr/bin/env python
import json
import logging
import platform
import os
import sys

NVIDIA_INSTALL_DEFAULT_PATH = "/usr/lib/updates"
XORG_MODULE_DIR = "/usr/lib/xorg/modules"
KERNEL_MODULE_DIR = "/lib/modules/" + platform.uname().release + "/updates"
LIBRARY_DIR = "/usr/lib"
VULKAN_DIR = "/usr/share/vulkan"

log = logging.getLogger(__name__)


class GpuError(Exception):
    """Exception raised when GPU not supported

    Attributes:
        product_id -- GPU searched
        message -- explanation of the error
    """

    def __init__(self, product_id, message):
        self.product_id = product_id
        self.message = message


class NvidiaInstaller:
    """Install nvidia driver on a system"""

    def __init__(self, driver_version: str):
        self.driver_version = driver_version
        self.versions = []
        self.search_path = NVIDIA_INSTALL_DEFAULT_PATH
        self.script_path = os.path.dirname(os.path.abspath(sys.argv[0]))
        self.gpus_data = {}
        self.gpus = []
        self.legacybranch = ""
        self.load_versions_file()

    def get_supported_gpus_file(self) -> str:
        """returns path to supported-gpus.json file"""
        return "/".join(
            (
                self.script_path,
                "supported-gpus.json"
            )
        )

    def get_driver_versions_list_file(self) -> str:
        """return path to versions file"""
        return "/".join(
            (
                self.search_path,
                "nvidia-" + self.driver_version,
                "versions"
            )
        )

    def set_search_path(self, path: str):
        self.search_path = path

    def install(self, product_id: str):
        """install nvidia driver"""
        self.load_supported_gpus_file()
        if self.is_gpu_supported(product_id):
            log.info(
                "Product %s is supported! Installing Nvidia proprietary driver version %s",
                product_id,
                self.driver_version,
            )
            self.do_install()
        else:
            raise GpuError(
                product_id=product_id, message="Controller not supported."
            )

    def get_file_list(self):
        """forge list of files/libs to install"""
        extra_search_path = self.search_path + "/nvidia-" + self.driver_version
        xorg_search_path = extra_search_path + "/xorg"
        mods_search_path = KERNEL_MODULE_DIR
        return {
            "files": [
                (
                    xorg_search_path + "/drivers/nvidia_drv.so",
                    XORG_MODULE_DIR + "/drivers/nvidia_drv.so",
                ),
                (
                    mods_search_path + "/nvidia.ko." + self.driver_version,
                    KERNEL_MODULE_DIR + "/nvidia.ko",
                ),
                (
                    mods_search_path + "/nvidia-drm.ko." + self.driver_version,
                    KERNEL_MODULE_DIR + "/nvidia-drm.ko",
                ),
                (
                    mods_search_path + "/nvidia-modeset.ko." + self.driver_version,
                    KERNEL_MODULE_DIR + "/nvidia-modeset.ko",
                ),
                (
                    mods_search_path + "/nvidia-uvm.ko." + self.driver_version,
                    KERNEL_MODULE_DIR + "/nvidia-uvm.ko",
                ),
                (
                    "/usr/share/glvnd/egl_vendor.d/10_nvidia.json."
                    + self.driver_version,
                    "/usr/share/glvnd/egl_vendor.d/10_nvidia.json",
                ),
                (
                    "/usr/share/glvnd/egl_vendor.d/10_nvidia_wayland.json."
                    + self.driver_version,
                    "/usr/share/glvnd/egl_vendor.d/10_nvidia_wayland.json",
                ),
                (
                    "/usr/share/X11/xorg.conf.d/10-nvidia-drm.conf."
                    + self.driver_version,
                    "/usr/share/X11/xorg.conf.d/10-nvidia-drm.conf",
                ),
                (
                    "/usr/bin/nvidia-settings." + self.driver_version,
                    "/usr/bin/nvidia-settings",
                ),
                ("/usr/bin/nvidia-smi." + self.driver_version, "/usr/bin/nvidia-smi"),
                (
                    "/usr/bin/nvidia-xconfig." + self.driver_version,
                    "/usr/bin/nvidia-xconfig",
                ),
                (
                    extra_search_path + "/nvidia_layers.json",
                    VULKAN_DIR + "/implicit_layers.d/nvidia_layers.json",
                ),
                (
                    extra_search_path + "/nvidia_icd.json",
                    VULKAN_DIR + "/icd.d/nvidia_icd.json",
                ),
            ],
            "libraries": [
                (
                    xorg_search_path + "/libnvidia-wfb.so",
                    XORG_MODULE_DIR + "/libnvidia-wfb.so",
                    [self.driver_version],
                ),
                (
                    xorg_search_path + "/extensions/libglx.so",
                    XORG_MODULE_DIR + "/extensions/libglx.so",
                    [self.driver_version],
                ),
                (
                    xorg_search_path + "/extensions/libglxserver_nvidia.so",
                    XORG_MODULE_DIR + "/extensions/libglxserver_nvidia.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libEGL.so",
                    LIBRARY_DIR + "/libEGL.so",
                    ["1.1.0"],
                ),
                (
                    extra_search_path + "/libEGL_nvidia.so",
                    LIBRARY_DIR + "/libEGL_nvidia.so",
                    [self.driver_version],
                ),
                (extra_search_path + "/libGL.so", LIBRARY_DIR + "/libGL.so", ["1.7.0"]),
                (
                    extra_search_path + "/libGLESv1_CM.so",
                    LIBRARY_DIR + "/libGLESv1_CM.so",
                    ["1.2.0"],
                ),
                (
                    extra_search_path + "/libGLESv1_CM_nvidia.so",
                    LIBRARY_DIR + "/libGLESv1_CM_nvidia.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libGLESv2.so",
                    LIBRARY_DIR + "/libGLESv2.so",
                    ["2.1.0"],
                ),
                (
                    extra_search_path + "/libGLESv2_nvidia.so",
                    LIBRARY_DIR + "/libGLESv2_nvidia.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libGLX_nvidia.so",
                    LIBRARY_DIR + "/libGLX_nvidia.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libOpenCL.so",
                    LIBRARY_DIR + "/libOpenCL.so",
                    ["1.0.0"],
                ),
                (
                    extra_search_path + "/libvdpau_nvidia.so",
                    LIBRARY_DIR + "/libvdpau_nvidia.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-egl-wayland.so",
                    LIBRARY_DIR + "/libnvidia-egl-wayland.so",
                    ["1.1.7", "1.1.9", "1.1.13", "1.1.18"],
                ),
                (extra_search_path + "/libGLX.so", LIBRARY_DIR + "/libGLX.so", ["0"]),
                (
                    extra_search_path + "/libGLdispatch.so",
                    LIBRARY_DIR + "/libGLdispatch.so",
                    ["0"],
                ),
                (
                    extra_search_path + "/libnvidia-eglcore.so",
                    LIBRARY_DIR + "/libnvidia-eglcore.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-glcore.so",
                    LIBRARY_DIR + "/libnvidia-glcore.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-glsi.so",
                    LIBRARY_DIR + "/libnvidia-glsi.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-glvkspirv.so",
                    LIBRARY_DIR + "/libnvidia-glvkspirv.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-tls.so",
                    LIBRARY_DIR + "/libnvidia-tls.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-gtk2.so",
                    LIBRARY_DIR + "/libnvidia-gtk2.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-gtk3.so",
                    LIBRARY_DIR + "/libnvidia-gtk3.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-ngx.so",
                    LIBRARY_DIR + "/libnvidia-ngx.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-ml.so",
                    LIBRARY_DIR + "/libnvidia-ml.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-cfg.so",
                    LIBRARY_DIR + "/libnvidia-cfg.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-allocator.so",
                    LIBRARY_DIR + "/libnvidia-allocator.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-api.so",
                    LIBRARY_DIR + "/libnvidia-api.so",
                    ["1"],
                ),
                (
                    extra_search_path + "/libnvidia-egl-gbm.so",
                    LIBRARY_DIR + "/libnvidia-egl-gbm.so",
                    ["1.1.1", "1.1.2"],
                ),
                (
                    extra_search_path + "/libnvidia-egl-xcb.so",
                    LIBRARY_DIR + "/libnvidia-egl-xcb.so",
                    ["1", "1.0.0"],
                ),
                (
                    extra_search_path + "/libnvidia-egl-xlib.so",
                    LIBRARY_DIR + "/libnvidia-egl-xlib.so",
                    ["1", "1.0.0"],
                ),
                (
                    extra_search_path + "/libnvidia-gpucomp.so",
                    LIBRARY_DIR + "/libnvidia-gpucomp.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-rtcore.so",
                    LIBRARY_DIR + "/libnvidia-rtcore.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-vksc-core.so",
                    LIBRARY_DIR + "/libnvidia-vksc-core.so",
                    [self.driver_version],
                ),
                (
                    extra_search_path + "/libnvidia-wayland-client.so",
                    LIBRARY_DIR + "/libnvidia-wayland-client.so",
                    [self.driver_version],
                ),
            ],
        }

    def do_install(self):
        """install files to their location"""
        file_list = self.get_file_list()
        for file in file_list["files"]:
            try:
                self.try_symlink(file[0], file[1])
            except IOError as e:
                log.error("Can't symlink %s to %s... skipping", file[0], file[1])
                log.error("Message was %s", e)
        for file in file_list["libraries"]:
            for version in file[2]:
                try:
                    self.try_symlink_lib(file[0], file[1], version)
                except IOError as e:
                    log.error("Can't symlink %s... skipping", file[0] + "." + version)
                    log.error("Message was %s", e)

    def load_versions_file(self):
        """
        load versions file
        this file is a space separated list of version strings of the driver
        examples:
        current production 560.35.03 560.xx
        """
        with open(
            self.get_driver_versions_list_file(), encoding="utf8"
        ) as versions_file:
            self.versions = versions_file.readline().split()

    def load_supported_gpus_file(self):
        """
        load supported-gpus.json file
        which is part of nvidia-driver-production-recalbox package
        """
        with open(
            self.get_supported_gpus_file(), encoding="utf8"
        ) as supported_gpus_file:
            self.gpus_data = json.load(supported_gpus_file)

    def is_gpu_supported(self, product_id: str):
        """
        Check if passed product_id is in the GPU driver versions list
        """
        if product_id.lower() in [item.lower() for version in self.versions if
                          version in self.gpus_data for item in self.gpus_data[version]]:
            return True
        return False

    def try_symlink(self, source: str, destination: str):
        """
        if source file exists, make a link to destination
        """
        if os.path.exists(source):
            log.debug("Processing %s", source)
            if os.path.exists(destination):
                log.debug("Remove %s", destination)
                os.unlink(destination)
            log.info("Link %s to %s", source, destination)
            os.symlink(source, destination)

    def try_symlink_lib(self, source: str, destination: str, version: str):
        """
        if source.version file exists, make a link to
        * destination.version
        * destination
        * destination.0
        * destination.1
        * destination.2
        """
        derivations = (
            "",
            "." + version,
            ".0",
            ".1",
            ".2",
        )
        if os.path.exists(source + "." + version):
            log.debug("Processing %s", source + "." + version)
            for derivation in derivations:
                if os.path.exists(destination + derivation):
                    log.debug("Remove %s", destination + derivation)
                    os.unlink(destination + derivation)
                log.info("Link %s.%s to %s", source, version, destination + derivation)
                os.symlink(source + "." + version, destination + derivation)
