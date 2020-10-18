from conans import ConanFile, CMake, tools
import os
import platform
import shutil

class VuoConan(ConanFile):
    generators = 'cmake'

    def requirements(self):
        self.requires('blackmagic/11.5.1-0@vuo/stable')
        self.requires('curl/7.65.3-0@vuo/stable')
        self.requires('discount/2.2.3a-1@vuo/stable')
        self.requires('ffmpeg/4.2.1-0@vuo/stable')
        self.requires('freeframe/1.6-1@vuo/stable')
        self.requires('freeimage/3.17.0-4@vuo/stable')
        self.requires('gamma/0.9.8-1@vuo/stable')
        self.requires('graphviz/2.28.0-5@vuo/stable')
        self.requires('jsonc/0.12-3@vuo/stable')
        self.requires('libcsv/3.0.3-4@vuo/stable')
        self.requires('libfacedetection/1-0@vuo/stable')
        self.requires('libfreenect/0.5.6-3@vuo/stable')
        self.requires('libfreenect2/0-6@vuo/stable')
        self.requires('liblqr/0.4.2-4@vuo/stable')
        self.requires('libxml2/2.9.2-3@vuo/stable')
        self.requires('llvm/3.3-6@vuo/stable')
        self.requires('muparser/2.2.5-3@vuo/stable')
        self.requires('ndi/4.5-1@vuo/stable')
        self.requires('oai/3.2-3@vuo/stable')
        self.requires('openssl/1.1.1c-0@vuo/stable')
        self.requires('oscpack/0-3@vuo/stable')
        self.requires('qt/5.11.3-1@vuo/stable')
        self.requires('rtaudio/4.1.2-3@vuo/stable')
        self.requires('rtmidi/2.0.1-3@vuo/stable')
        self.requires('wjelement/1.3-1@vuo/stable')
        self.requires('zeromq/2.2.0-4@vuo/stable')
        self.requires('zlib/1.2.11-1@vuo/stable')
        self.requires('zxing/0-3@vuo/stable')

        if platform.system() == 'Darwin':
            self.requires('ld64/253.3-0@vuo/stable')
            self.requires('cctools/921-1@vuo/stable')
            self.requires('csu/85-1@vuo/stable')
            self.requires('macos-sdk/10.11-0@vuo/stable')

        elif platform.system() == 'Linux':
            # These are system libraries on macOS, but they aren't provided by the system on Linux.
            self.requires("libdispatch/4.0.3-1@vuo/stable")

        else:
            raise Exception('Unknown platform "%s"' % platform.system())

    def imports(self):
        self.copy('*'      , src='bin', dst='bin', excludes=[
            'Assistant.app*',
            'Designer.app*',
            'fixqt4headers.pl',
            'lconvert',
            'macchangeqt',
            'macdeployqt',
            'pixeltool.app*',
            'qcollectiongenerator',
            'qdoc',
            'qhelpconverter',
            'qhelpgenerator',
            'qlalr',
            'qml.app*',
            'qtdiag',
            'qtpaths',
            'qtplugininfo',
            'syncqt.pl',
        ])
        self.copy('*', src='plugins', dst='lib/QtPlugins')

        # Create qt.conf so Qt knows where to find its plugins.
        for f in [
            'bin/qt.conf',
            'bin/Linguist.app/Contents/MacOS/qt.conf',
        ]:
            tools.save(f, "[Paths]\nPrefix = %s" % self.deps_cpp_info["qt"].rootpath)

        # Enable Qt tools to find the frameworks they depend on.
        for f in [
            'Linguist.app/Contents/MacOS/Linguist',
            'lrelease',
            'lupdate',
            'qmlcachegen',
            'qmleasing',
            'qmlimportscanner',
            'qmllint',
            'qmlmin',
            'qmlplugindump',
            'qmlscene',
            'qmltestrunner',
            'qtattributionsscanner',
            'qvkgen',
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
                self.run('install_name_tool -change @rpath/%s.framework/Versions/5/%s %s/lib/%s.framework/Versions/5/%s bin/%s'
                    % (l, l, self.deps_cpp_info["qt"].rootpath, l, l, f))

        self.copy('*', src='license', dst='license')
        self.run('chmod u=rw,go=r license/*.txt')
