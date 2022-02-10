from conans import ConanFile, CMake, tools
from conans.model.settings import Settings
from conan.tools.cmake import CMakeToolchain
import os
import platform
import shutil

class VuoConan(ConanFile):
    settings_build = Settings()  # Quell warning about not selecting a profile.
    generators = 'cmake'

    def requirements(self):
        self.requires('blackmagic/12.0-0@vuo+conan+blackmagic/stable')
        self.requires('curl/7.73.0-1@vuo+conan+curl/stable')
        self.requires('discount/2.2.6-0@vuo+conan+discount/stable')
        self.requires('ffmpeg/4.4-3@vuo+conan+ffmpeg/stable')
        self.requires('freeframe/1.6-1@vuo+conan+freeframe/stable')
        self.requires('freeimage/3.18.0-0@vuo+conan+freeimage/stable')
        self.requires('gamma/0.9.8-2@vuo+conan+gamma/stable')
        self.requires('gettext/0.21-0@vuo+conan+gettext/stable')
        self.requires('glib/2.66.2-0@vuo+conan+glib/stable')
        self.requires('graphviz/2.44.1-1@vuo+conan+graphviz/stable')
        self.requires('jsonc/0.15-0@vuo+conan+jsonc/stable')
        self.requires('libcsv/3.0.3-5@vuo+conan+libcsv/stable')
        self.requires('libfacedetection/1-1@vuo+conan+libfacedetection/stable')
        self.requires('libffi/3.4pre-0@vuo+conan+libffi/stable')
        self.requires('libfreenect/0.6.1-0@vuo+conan+libfreenect/stable')
        self.requires('libfreenect2/0-7@vuo+conan+libfreenect2/stable')
        self.requires('liblqr/0.4.2-5@vuo+conan+liblqr/stable')
        self.requires('libusb/1.0.23-0@vuo+conan+libusb/stable')
        self.requires('libxml2/2.9.10-0@vuo+conan+libxml2/stable')
        self.requires('llvm/5.0.2-5@vuo+conan+llvm/stable')
        self.requires('muparser/2.3.2-0@vuo+conan+muparser/stable')
        self.requires('ndi/5.0.0-0@vuo+conan+ndi/stable')
        self.requires('oai/5.0.1-1@vuo+conan+oai/stable')
        self.requires('openssl/1.1.1h-0@vuo+conan+openssl/stable')
        self.requires('oscpack/0-5@vuo+conan+oscpack/stable')
        self.requires('qt/5.12.11-3@vuo+conan+qt/stable')
        self.requires('rtaudio/5.2.0-0@vuo+conan+rtaudio/stable')
        self.requires('rtmidi/4.0.0-1@vuo+conan+rtmidi/stable')
        self.requires('wjelement/1.3-2@vuo+conan+wjelement/stable')
        self.requires('zeromq/4.3.3-0@vuo+conan+zeromq/stable')
        self.requires('zlib/1.2.11-2@vuo+conan+zlib/stable')
        self.requires('zxing/0-4@vuo+conan+zxing/stable')

        if platform.system() == 'Darwin':
            self.requires('ld64/530-5@vuo+conan+ld64/stable')
            self.requires('cctools/949.0.1-2@vuo+conan+cctools/stable')
            self.requires('codesign_allocate/10.3+12.4-0@vuo+conan+codesign_allocate/stable')
            self.requires('fxplug/4.2.2-1@vuo+conan+fxplug/stable')
            self.requires('hap/1.5.3-1@vuo+conan+hap/stable')
            self.requires('macos-sdk/11.0-0@vuo+conan+macos-sdk/stable')
            self.requires('syphon/5-1@vuo+conan+syphon/stable')

        elif platform.system() == 'Linux':
            # These are system libraries on macOS, but they aren't provided by the system on Linux.
            self.requires('libdispatch/4.0.3-1@vuo+conan+libdispatch/stable')

        else:
            raise Exception('Unknown platform "%s"' % platform.system())

    def imports(self):
        self.copy('*', src='bin', dst='bin', excludes=['lconvert'])
        self.copy('*', src='plugins', dst='lib/QtPlugins')

        # Create qt.conf so Qt knows where to find its plugins.
        for f in [
            'bin/qt.conf',
        ]:
            tools.save(f, "[Paths]\nPrefix = %s" % self.deps_cpp_info["qt"].rootpath)

        # Enable Qt tools to find the frameworks they depend on.
        for f in [
            'lrelease',
            'lupdate',
            'uic',
        ]:
            for l in [
                'QtCore',
                'QtGui',
                'QtNetwork',
                'QtPrintSupport',
                'QtQml',
                'QtQuick',
                'QtQuickTest',
                'QtTest',
                'QtWidgets',
                'QtXml',
            ]:
                self.run('install_name_tool -change @rpath/%s.framework/Versions/5/%s \
                          %s/lib/%s.framework/Versions/5/%s bin/%s \
                          2>&1 | (grep -F -v "will invalidate the code signature" || true)'
                    % (l, l, self.deps_cpp_info["qt"].rootpath, l, l, f))
            self.run('codesign --sign - --force bin/%s \
                      2>&1 | (grep -F -v ": replacing existing signature" || true)' % f)

        self.copy('*', src='license', dst='license')
        self.run('chmod u=rw,go=r license/*.txt')

        # compiler-rt isn't supported on arm64 until LLVM/Clang 11.
        # For now, delete it to avoid "missing required architecture arm64" warnings.
        # https://reviews.llvm.org/D82610
        self.run('rm -f %s/lib/clang/5.0.2/lib/darwin/libclang_rt.osx.a'
                 % self.deps_cpp_info["llvm"].rootpath)

    def generate(self):
        tc = CMakeToolchain(self)
        for k, v in self.deps_cpp_info.dependencies:
            tc.variables["CONAN_%s_VERSION" % k] = v.version.split('-')[0]
        tc.generate()
