/**
 * @file
 * VuoCompilerTargetSet interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

/**
 * Stores a set of targets (operating system versions) and can be used to check compatibility.
 */
class VuoCompilerTargetSet
{
public:
	/**
	 * Mac OS versions.
	 */
	enum MacVersion
	{
		MacVersion_Any,
		MacVersion_10_7,
		MacVersion_10_8,
		MacVersion_10_9,
		MacVersion_10_10,
		MacVersion_10_11,
		MacVersion_10_12,
	};

	VuoCompilerTargetSet(void);
	void setMinMacVersion(enum MacVersion min);
	void setMaxMacVersion(enum MacVersion max);
	void restrictToCurrentOperatingSystemVersion(void);
	void restrictToBeCompatibleWithAllOf(const VuoCompilerTargetSet &other);
	bool isCompatibleWithAllOf(const VuoCompilerTargetSet &other) const;
	string toString(void);

private:
	pair<enum MacVersion, enum MacVersion> macVersionRange;  ///< The minimum and maximum Mac OS version

	string macVersionToString(MacVersion v);
};
