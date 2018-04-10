MACOSX_SDK_FOLDER = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
exists     ($$MACOSX_SDK_FOLDER/MacOSX10.8.sdk)  { QMAKE_MAC_SDK.macosx.path = $$MACOSX_SDK_FOLDER/MacOSX10.8.sdk  }
else:exists($$MACOSX_SDK_FOLDER/MacOSX10.9.sdk)  { QMAKE_MAC_SDK.macosx.path = $$MACOSX_SDK_FOLDER/MacOSX10.9.sdk  }
else:exists($$MACOSX_SDK_FOLDER/MacOSX10.10.sdk) { QMAKE_MAC_SDK.macosx.path = $$MACOSX_SDK_FOLDER/MacOSX10.10.sdk }
else { error("No usable SDK was found in "$$MACOSX_SDK_FOLDER".  Vuo currently needs MacOSX10.8.sdk, MacOSX10.9.sdk, or MacOSX10.10.sdk.") }
