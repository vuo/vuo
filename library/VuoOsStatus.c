/**
 * @file
 * VuoOsStatus implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoOsStatus.h"

#include "module.h"

#include <CoreServices/CoreServices.h>
#include <CoreVideo/CVReturn.h>
#include <CoreMediaIO/CMIOHardware.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
					  "title" : "VuoOsStatus",
					  "dependencies" : [
						  "CoreFoundation.framework"
					  ]
				 });
#endif

/**
 * Returns a string providing a (slightly) better explanation of `error`.
 *
 * The caller is responsible for freeing the returned string.
 */
char *VuoOsStatus_getText(int32_t error)
{
	if (error == 0)
		return strdup("(no error)");
	else if (error == bdNamErr)
		return strdup("Bad file name");
	else if (error == fnfErr)
		return strdup("File not found");

	// LSInfo.h
	else if (error == kLSAppInTrashErr)                              return strdup("The app cannot be run when inside a Trash folder");
	else if (error == kLSExecutableIncorrectFormat)                  return strdup("No compatible executable was found");
	else if (error == kLSAttributeNotFoundErr)                       return strdup("An item attribute value could not be found with the specified name");
	else if (error == kLSAttributeNotSettableErr)                    return strdup("The attribute is not settable");
	else if (error == kLSIncompatibleApplicationVersionErr)          return strdup("The app is incompatible with the current OS");
	else if (error == kLSNoRosettaEnvironmentErr)                    return strdup("The Rosetta environment was required not available");
	else if (error == -10666 /*kLSGarbageCollectionUnsupportedErr*/) return strdup("Objective-C garbage collection is no longer supported");
	else if (error == kLSUnknownErr)                                 return strdup("Unexpected internal error");
	else if (error == kLSNotAnApplicationErr)                        return strdup("Item needs to be an application, but is not");
	else if (error == kLSNotInitializedErr)                          return strdup("Not initialized");
	else if (error == kLSDataUnavailableErr)                         return strdup("Data unavailable (e.g., no kind string)");
	else if (error == kLSApplicationNotFoundErr)                     return strdup("Application not found (e.g., no application claims the file)");
	else if (error == kLSUnknownTypeErr)                             return strdup("Don't know anything about the type of the item");
	else if (error == kLSDataTooOldErr)                              return strdup("Data too old");
	else if (error == kLSDataErr)                                    return strdup("Data error");
	else if (error == kLSLaunchInProgressErr)                        return strdup("Attempted to launch an already launching application");
	else if (error == kLSNotRegisteredErr)                           return strdup("Not registered");
	else if (error == kLSAppDoesNotClaimTypeErr)                     return strdup("One or more documents are of types (and/or one or more URLs are of schemes) not supported by the target application");
	else if (error == kLSAppDoesNotSupportSchemeWarning)             return strdup("The app does not support this scheme");
	else if (error == kLSServerCommunicationErr)                     return strdup("The server process (registration and recent items) is not available");
	else if (error == kLSCannotSetInfoErr)                           return strdup("The extension visibility on this item cannot be changed");
	else if (error == kLSNoRegistrationInfoErr)                      return strdup("The item contains no registration info");
	else if (error == kLSIncompatibleSystemVersionErr)               return strdup("The app cannot run on the current OS version");
	else if (error == kLSNoLaunchPermissionErr)                      return strdup("User doesn't have permission to launch the app (managed networks)");
	else if (error == kLSNoExecutableErr)                            return strdup("The executable is missing");
	else if (error == kLSNoClassicEnvironmentErr)                    return strdup("The Classic environment was required but is not available");
	else if (error == kLSMultipleSessionsNotSupportedErr)            return strdup("The app cannot run simultaneously in two different sessions");

	// CVReturn.h
	else if (error == kCVReturnInvalidArgument)                         return strdup("At least one of the arguments passed in is not valid. Either out of range or the wrong type.");
	else if (error == kCVReturnAllocationFailed)                        return strdup("The allocation for a buffer or buffer pool failed. Most likely because of lack of resources.");
	else if (error == -6663 /*kCVReturnUnsupported*/)                   return strdup("Unsupported.");
	else if (error == kCVReturnInvalidDisplay)                          return strdup("A CVDisplayLink cannot be created for the given DisplayRef.");
	else if (error == kCVReturnDisplayLinkAlreadyRunning)               return strdup("The CVDisplayLink is already started and running.");
	else if (error == kCVReturnDisplayLinkNotRunning)                   return strdup("The CVDisplayLink has not been started.");
	else if (error == kCVReturnDisplayLinkCallbacksNotSet)              return strdup("The output callback is not set.");
	else if (error == kCVReturnInvalidPixelFormat)                      return strdup("The requested pixelformat is not supported for the CVBuffer type.");
	else if (error == kCVReturnInvalidSize)                             return strdup("The requested size (most likely too big) is not supported for the CVBuffer type.");
	else if (error == kCVReturnInvalidPixelBufferAttributes)            return strdup("A CVBuffer cannot be created with the given attributes.");
	else if (error == kCVReturnPixelBufferNotOpenGLCompatible)          return strdup("The Buffer cannot be used with OpenGL as either its size, pixelformat or attributes are not supported by OpenGL.");
	else if (error == -6684 /*kCVReturnPixelBufferNotMetalCompatible*/) return strdup("The Buffer cannot be used with Metal as either its size, pixelformat or attributes are not supported by Metal.");
	else if (error == kCVReturnWouldExceedAllocationThreshold)          return strdup("The allocation request failed because it would have exceeded a specified allocation threshold (see kCVPixelBufferPoolAllocationThresholdKey).");
	else if (error == kCVReturnPoolAllocationFailed)                    return strdup("The allocation for the buffer pool failed. Most likely because of lack of resources. Check if your parameters are in range.");
	else if (error == kCVReturnInvalidPoolAttributes)                   return strdup("A CVBufferPool cannot be created with the given attributes.");
	else if (error == -6692 /*kCVReturnRetry*/)                         return strdup("A scan hasn't completely traversed the CVBufferPool due to a concurrent operation. The client can retry the scan.");

	// CMIOHardware.h
	// AudioHardwareBase.h
	else if (error == kCMIOHardwareNotStoppedError)           return strdup("The function call requires that the hardware be stopped but it isn't.");
	else if (error == kCMIOHardwareNotRunningError)           return strdup("The function call requires that the hardware be running but it isn't.");
	else if (error == kCMIOHardwareUnspecifiedError)          return strdup("The function call failed while doing something that doesn't provide any error messages.");
	else if (error == kCMIOHardwareUnknownPropertyError)      return strdup("The object doesn't know about the property at the given address.");
	else if (error == kCMIOHardwareBadPropertySizeError)      return strdup("An improperly sized buffer was provided when accessing the data of a property.");
	else if (error == kCMIOHardwareIllegalOperationError)     return strdup("The requested operation couldn't be completed.");
	else if (error == kCMIOHardwareBadObjectError)            return strdup("The CMIOObjectID passed to the function doesn't map to a valid CMIOObject.");
	else if (error == kCMIOHardwareBadDeviceError)            return strdup("The CMIODeviceID passed to the function doesn't map to a valid CMIODevice.");
	else if (error == kCMIOHardwareBadStreamError)            return strdup("The CMIODeviceID passed to the function doesn't map to a valid CMIOStream.");
	else if (error == kCMIOHardwareUnsupportedOperationError) return strdup("The device doesn't support the requested operation.");
	else if (error == kCMIOHardwareSuspendedBySystemError)    return strdup("The function call failed because because access been suspended by the system.");
	else if (error == kCMIODeviceUnsupportedFormatError)      return strdup("The CMIOStream doesn't support the requested format.");
	else if (error == kCMIODevicePermissionsError)            return strdup("The requested operation can't be completed because the process doesn't have permission.");

	// AudioCodec.h
	else if (error == 561214580 /*kAudioCodecStateError*/)                return strdup("Codec state error.");
	else if (error == 560100710 /*kAudioCodecNotEnoughBufferSpaceError*/) return strdup("Not enough buffer space.");

	// CGError.h
	else if (error == kCGErrorFailure)           return strdup("Failure.");
	else if (error == kCGErrorIllegalArgument)   return strdup("Illegal argument.");
	else if (error == kCGErrorInvalidConnection) return strdup("Invalid connection.");
	else if (error == kCGErrorInvalidContext)    return strdup("Invalid context.");
	else if (error == kCGErrorCannotComplete)    return strdup("Cannot complete.");
	else if (error == kCGErrorNotImplemented)    return strdup("Not implemented.");
	else if (error == kCGErrorRangeCheck)        return strdup("Range check.");
	else if (error == kCGErrorTypeCheck)         return strdup("Type check.");
	else if (error == kCGErrorInvalidOperation)  return strdup("Invalid operation.");
	else if (error == kCGErrorNoneAvailable)     return strdup("None available.");

	// CSCommon.h
	else if (error == errSecCSUnimplemented)                return strdup("unimplemented code signing feature");
	else if (error == errSecCSInvalidObjectRef)             return strdup("invalid API object reference");
	else if (error == errSecCSInvalidFlags)                 return strdup("invalid or inappropriate API flag(s) specified");
	else if (error == errSecCSObjectRequired)               return strdup("a required pointer argument was NULL");
	else if (error == errSecCSStaticCodeNotFound)           return strdup("cannot find code object on disk");
	else if (error == errSecCSUnsupportedGuestAttributes)   return strdup("cannot locate guests using this attribute set");
	else if (error == errSecCSInvalidAttributeValues)       return strdup("given attribute values are invalid");
	else if (error == errSecCSNoSuchCode)                   return strdup("host has no guest with the requested attributes");
	else if (error == errSecCSMultipleGuests)               return strdup("ambiguous guest specification (host has multiple guests with these attribute values)");
	else if (error == errSecCSGuestInvalid)                 return strdup("code identity has been invalidated");
	else if (error == errSecCSUnsigned)                     return strdup("code object is not signed at all");
	else if (error == errSecCSSignatureFailed)              return strdup("invalid signature (code or signature have been modified)");
	else if (error == errSecCSSignatureNotVerifiable)       return strdup("the code cannot be read by the verifier (file system permissions etc.)");
	else if (error == errSecCSSignatureUnsupported)         return strdup("unsupported type or version of signature");
	else if (error == errSecCSBadDictionaryFormat)          return strdup("a required plist file or resource is malformed");
	else if (error == errSecCSResourcesNotSealed)           return strdup("resources are present but not sealed by signature");
	else if (error == errSecCSResourcesNotFound)            return strdup("code has no resources but signature indicates they must be present");
	else if (error == errSecCSResourcesInvalid)             return strdup("the sealed resource directory is invalid");
	else if (error == errSecCSBadResource)                  return strdup("a sealed resource is missing or invalid");
	else if (error == errSecCSResourceRulesInvalid)         return strdup("invalid resource specification rule(s)");
	else if (error == errSecCSReqInvalid)                   return strdup("invalid or corrupted code requirement(s)");
	else if (error == errSecCSReqUnsupported)               return strdup("unsupported type or version of code requirement(s)");
	else if (error == errSecCSReqFailed)                    return strdup("code failed to satisfy specified code requirement(s)");
	else if (error == errSecCSBadObjectFormat)              return strdup("object file format unrecognized, invalid, or unsuitable");
	else if (error == errSecCSInternalError)                return strdup("internal error in Code Signing subsystem");
	else if (error == errSecCSHostReject)                   return strdup("code rejected its host");
	else if (error == errSecCSNotAHost)                     return strdup("attempt to specify guest of code that is not a host");
	else if (error == errSecCSSignatureInvalid)             return strdup("invalid or unsupported format for signature");
	else if (error == errSecCSHostProtocolRelativePath)     return strdup("host protocol violation - absolute guest path required");
	else if (error == errSecCSHostProtocolContradiction)    return strdup("host protocol violation - contradictory hosting modes");
	else if (error == errSecCSHostProtocolDedicationError)  return strdup("host protocol violation - operation not allowed with/for a dedicated guest");
	else if (error == errSecCSHostProtocolNotProxy)         return strdup("host protocol violation - proxy hosting not engaged");
	else if (error == errSecCSHostProtocolStateError)       return strdup("host protocol violation - invalid guest state change request");
	else if (error == errSecCSHostProtocolUnrelated)        return strdup("host protocol violation - the given guest is not a guest of the given host");
	else if (error == errSecCSNotSupported)                 return strdup("operation inapplicable or not supported for this type of code");
	else if (error == errSecCSCMSTooLarge)                  return strdup("signature too large to embed (size limitation of on-disk representation)");
	else if (error == errSecCSHostProtocolInvalidHash)      return strdup("host protocol violation - invalid guest hash");
	else if (error == errSecCSStaticCodeChanged)            return strdup("the code on disk does not match what is running");
	else if (error == errSecCSDBDenied)                     return strdup("permission to use a database denied");
	else if (error == errSecCSDBAccess)                     return strdup("cannot access a database");
	else if (error == errSecCSSigDBDenied)                  return strdup("SigDB denied");
	else if (error == errSecCSSigDBAccess)                  return strdup("SigDB access");
	else if (error == errSecCSHostProtocolInvalidAttribute) return strdup("host returned invalid or inconsistent guest attributes");
	else if (error == errSecCSInfoPlistFailed)              return strdup("invalid Info.plist (plist or signature have been modified)");
	else if (error == errSecCSNoMainExecutable)             return strdup("the code has no main executable file");
	else if (error == errSecCSBadBundleFormat)              return strdup("bundle format unrecognized, invalid, or unsuitable");
	else if (error == errSecCSNoMatches)                    return strdup("no matches for search or update operation");
	else if (error == errSecCSFileHardQuarantined)          return strdup("File created by an AppSandbox, exec/open not allowed");
	else if (error == errSecCSOutdated)                     return strdup("presented data is out of date");
	else if (error == errSecCSDbCorrupt)                    return strdup("a system database or file is corrupt");
	else if (error == errSecCSResourceDirectoryFailed)      return strdup("invalid resource directory (directory or signature have been modified)");
	else if (error == errSecCSUnsignedNestedCode)           return strdup("nested code is unsigned");
	else if (error == errSecCSBadNestedCode)                return strdup("nested code is modified or invalid");
	else if (error == errSecCSBadCallbackValue)             return strdup("monitor callback returned invalid value");
	else if (error == errSecCSHelperFailed)                 return strdup("the codesign_allocate helper tool cannot be found or used");
	else if (error == errSecCSVetoed)                       return strdup("vetoed");
	else if (error == errSecCSBadLVArch)                    return strdup("library validation flag cannot be used with an i386 binary");
	else if (error == errSecCSResourceNotSupported)         return strdup("unsupported resource found (something not a directory, file or symlink)");
	else if (error == errSecCSRegularFile)                  return strdup("the main executable or Info.plist must be a regular file (no symlinks, etc.)");
	else if (error == errSecCSUnsealedAppRoot)              return strdup("unsealed contents present in the bundle root");
	else if (error == errSecCSWeakResourceRules)            return strdup("resource envelope is obsolete (custom omit rules)");
	else if (error == errSecCSDSStoreSymlink)               return strdup(".DS_Store files cannot be a symlink ");
	else if (error == errSecCSAmbiguousBundleFormat)        return strdup("bundle format is ambiguous (could be app or framework)");
	else if (error == errSecCSBadMainExecutable)            return strdup("main executable failed strict validation");
	else if (error == errSecCSBadFrameworkVersion)          return strdup("embedded framework contains modified or invalid version");
	else if (error == errSecCSUnsealedFrameworkRoot)        return strdup("unsealed contents present in the root directory of an embedded framework");
	else if (error == errSecCSWeakResourceEnvelope)         return strdup("resource envelope is obsolete (version 1 signature)");
	else if (error == errSecCSCancelled)                    return strdup("operation was terminated by explicit cancellation");

	char *errorString = (char *)calloc(1, 12);  // ceil(log10(INT_MAX)) + 1 (optional negative sign) + 1 (null terminator)

	*(UInt32 *)(errorString + 1) = CFSwapInt32HostToBig(error);
	if (isprint(errorString[1]) && isprint(errorString[2]) && isprint(errorString[3]) && isprint(errorString[4]))
	{
		errorString[0] = errorString[5] = '\'';
		errorString[6] = '\0';
	}
	else
		sprintf(errorString, "%d", (int)error);

	return errorString;
}
