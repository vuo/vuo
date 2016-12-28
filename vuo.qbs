import qbs 1.0
import qbs.File

Project {
	property string version: '1.2.0'

	property var paths: ({
		llvm:        '/usr/local/Cellar/llvm/3.2',
		jsonc:       '/usr/local/Cellar/json-c/0.12',
		graphviz:    '/usr/local/Cellar/graphviz/2.28.0',
		qt:          '/usr/local/Cellar/qt/5.3.1',
		libffi:      '/usr/local/Cellar/libffi/3.0.11',
		zlib:        '/usr/local/Cellar/zlib/1.2.8',
		zmq:         '/usr/local/Cellar/zeromq/2.2.0',
		openssl:     '/usr/local/Cellar/openssl/1.0.1g',
		muparser:    '/usr/local/Cellar/muparser/2.2.3',
		freeimage:   '/usr/local/Cellar/freeimage/3.15.4',
		curl:        '/usr/local/Cellar/curl/7.30.0',
		rtmidi:      '/usr/local/Cellar/rtmidi/2.0.1',
		rtaudio:     '/usr/local/Cellar/rtaudio/4.0.12',
		gamma:       '/usr/local/Cellar/gamma/0.9.5',
		assimp:      '/usr/local/Cellar/assimp/3.1.1',
		discount:    '/usr/local/Cellar/discount/2.1.6',
		ffmpeg:      '/usr/local/Cellar/ffmpeg/2.1',
		libusb:      '/usr/local/Cellar/libusb/1.0.9',
		libfreenect: '/usr/local/Cellar/libfreenect/0.2.0',
		oscpack:     '/usr/local/Cellar/oscpack/1.1.0',
		zxing:       '/usr/local/Cellar/zxing/2.3.0',
		libxml2:     '/usr/local/Cellar/libxml2/2.9.2',
		ghostscript: '/usr/local/Cellar/ghostscript/9.15',
		pngquant:    '/usr/local/Cellar/pngquant/2.3.1',
	})

	property bool premiumAvailable: false
	Properties {
		condition: { return File.exists('licensetools'); }
		premiumAvailable: true
	}

	minimumQbsVersion: '1.4.2'
	qbsSearchPaths: 'qbs'

	references: [
		'pch',
		'base',
		'compiler',
	]
}
