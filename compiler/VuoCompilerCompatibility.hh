/**
 * @file
 * VuoCompilerCompatibility interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Immutable representation of a set of platforms, OS versions, and architectures,
 * used for expressing a module's compatibility with a subset of Vuo's supported targets
 * and checking if the module is compatible with a given target.
 */
class VuoCompilerCompatibility
{
public:
	explicit VuoCompilerCompatibility(json_object *json);
	VuoCompilerCompatibility(const VuoCompilerCompatibility &other);
	VuoCompilerCompatibility& operator=(const VuoCompilerCompatibility &other);
	~VuoCompilerCompatibility(void);

	bool isCompatibleWith(const VuoCompilerCompatibility &other);
	VuoCompilerCompatibility intersection(const VuoCompilerCompatibility &other);
	string toString(void);
	string toJsonString(void);
	bool isCompatibleWithPlatform(const string &platform);
	string getMinVersionOnPlatform(const string &platform);

	static VuoCompilerCompatibility compatibilityWithAnySystem(void);
	static VuoCompilerCompatibility compatibilityWithArchitectures(const set<string> &architectures);
	static VuoCompilerCompatibility compatibilityWithTargetTriple(const string &target);

private:
	/**
	 * A null value means compatible with any target.
	 *
	 * A non-null values specifies restrictions for specific platforms (subkey `macos`, `windows`, `linux`),
	 * OS versions (subsubkeys `min`/`max`), and CPU architectures (subsubkey `arch`). Platforms that aren't
	 * specified remain unrestricted.
	 *
	 * @eg{
	 * // On macOS 10.11-10.12 it's unavailable
	 * // On macOS 10.13+ it's available on both x86_64 and arm64
	 * // On Windows (etc.) it's available on all OS versions and CPU architectures
	 * {
	 *  "compatibility": {
	 *   "macos": {
	 *      "min": "10.13",
	 *   },
	 *  },
	 * }
	 * }
	 *
	 * @eg{
	 * // On macOS it's available only on x86_64 (not arm64)
	 * // On Windows it's unavailable
	 * {
	 *  "compatibility": {
	 *   "macos": {
	 *      "arch": ["x86_64"],
	 *   },
	 *   "windows": false,
	 *  },
	 * }
	 * }
	 *
	 * @eg{
	 * // On macOS it's unavailable
	 * // On Windows (etc) it's available on all OS versions and CPU architectures
	 * {
	 *  "compatibility": {
	 *   "macos": false,
	 *  },
	 * }
	 * }
	 */
	json_object *json;

	json_object * findPlatform(json_object *json, string platformKey, bool &isPlatformCompatible);
	string findVersion(json_object *platformVal, const string &minOrMax);
	vector<string> findArchitectures(json_object *platformVal);

	static const map<string, string> knownPlatforms;  ///< 'compatibility' keys and human-readable names of platforms recognized by this class
	static const map<string, string> knownArchitectures;  ///< 'compatibility' keys and human-readable names of architectures recognized by this class
};
