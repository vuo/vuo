/**
 * @file
 * VuoCompilerTargetSet interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Stores a set of targets (operating system versions) and can be used to check compatibility.
 */
class VuoCompilerTargetSet
{
public:
	/**
	 * macOS versions.
	 */
	enum MacVersion
	{
		MacVersion_Any,
		MacVersion_10_11,
		MacVersion_10_12,
		MacVersion_10_13,
		MacVersion_10_14,
		MacVersion_10_15,
		MacVersion_11_0,
	};

	VuoCompilerTargetSet(void);

	void setMinMacVersion(enum MacVersion min);
	string getMinMacVersionString();

	void setMaxMacVersion(enum MacVersion max);

	void restrictToCurrentOperatingSystemVersion(void);
	void restrictToBeCompatibleWithAllOf(const VuoCompilerTargetSet &other);

	bool isCompatibleWithAllOf(const VuoCompilerTargetSet &other) const;

	string toString(void);

private:
	pair<enum MacVersion, enum MacVersion> macVersionRange;  ///< The minimum and maximum macOS version

	string macVersionToString(MacVersion v);
};
