from conans import ConanFile, tools, CMake
import os

class GlfwConan(ConanFile):
    name = "glfw"
    version = "3.3.2"
    description = "A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input https://www.glfw.org/"
    homepage = "https://github.com/glfw/glfw"
    url = "https://github.com/zyperpl"
    license = "Zlib"
    author = "MANCIAUX Romain (https://github.com/PamplemousseMR); Zyper (github.com/zyperpl)"
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"
    options = { "shared": [True, False], "fPIC": [True, False] }
    default_options = { "shared": False, "fPIC": True }
    exports = "LICENSE.md"
    short_paths=True
    
    _source_folder = "{0}-{1}_sources".format(name, version)
    _build_folder = "{0}-{1}_build".format(name, version)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        del self.settings.compiler.libcxx

    def source(self):
        tools.get("{0}/archive/{1}.tar.gz".format(self.homepage, self.version), sha256="98768e12e615fbe9f3386f5bbfeb91b5a3b45a8c4c77159cef06b1f6ff749537")
        os.rename("{0}-{1}".format(self.name, self.version), self._source_folder)

    def build(self):
        cmake = CMake(self)
        cmake.definitions["GLFW_BUILD_EXAMPLES"] = False
        cmake.definitions["GLFW_BUILD_TESTS"] = False
        cmake.definitions["GLFW_BUILD_DOCS"] = False
        cmake.configure(source_folder=self._source_folder, build_folder=self._build_folder)
        cmake.build()
        cmake.install()

    def package(self):
        self.copy(pattern="*.pdb", dst="bin", keep_path=False)        
        for export in self.exports:
            self.copy(export, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.libs.extend(['Xrandr', 'Xrender', 'Xi', 'Xinerama', 'Xcursor', 'GL', 'm', 'dl', 'drm', 'Xdamage', 'X11-xcb', 'xcb-glx', 'xcb-dri2', 'xcb-dri3', 'xcb-present', 'xcb-sync', 'Xxf86vm', 'Xfixes', 'Xext', 'X11', 'pthread', 'xcb', 'Xau'])
        elif self.settings.os == "Macos":
            self.cpp_info.exelinkflags.append("-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")