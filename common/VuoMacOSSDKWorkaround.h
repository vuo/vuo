/**
 * @file
 * VuoMacOSSDKWorkaround header.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Clang 5 doesn't understand the macOS 11 SDK's availability macros; replace them with our own simpler ones.
 */
#define API_AVAILABLE(...)
#define API_UNAVAILABLE(...)                                                                                                   ///< @copydoc API_AVAILABLE
#define API_UNAVAILABLE_BEGIN(...)                                                                                             ///< @copydoc API_AVAILABLE
#define API_UNAVAILABLE_END                                                                                                    ///< @copydoc API_AVAILABLE
#define API_DEPRECATED(message, ...) __attribute__((deprecated(message)))                                                      ///< @copydoc API_AVAILABLE
#define API_DEPRECATED_WITH_REPLACEMENT(replacement, ...) __attribute__((deprecated("Deprecated; replaced by " replacement)))  ///< @copydoc API_AVAILABLE
#define __OS_AVAILABILITY__                                                                                                    ///< @copydoc API_AVAILABLE
#define API_TO_BE_DEPRECATED 100000                                                                                            ///< @copydoc API_AVAILABLE
#define UT_AVAILABLE_BEGIN                                                                                                     ///< @copydoc API_AVAILABLE
#define UT_AVAILABLE_END                                                                                                       ///< @copydoc API_AVAILABLE
#ifndef NS_RETURNS_INNER_POINTER
#define NS_RETURNS_INNER_POINTER                                                                                               ///< @copydoc API_AVAILABLE
#endif
#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif
