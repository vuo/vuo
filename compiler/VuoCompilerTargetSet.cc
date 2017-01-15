/**
 * @file
 * VuoCompilerTargetSet implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerTargetSet.hh"

#include <CoreServices/CoreServices.h>

/**
 * Creates a target set that is unrestricted. It includes any operating system version.
 */
VuoCompilerTargetSet::VuoCompilerTargetSet(void)
{
	macVersionRange = make_pair(MacVersion_Any, MacVersion_Any);
}

/**
 * Restricts the target set to Mac OS versions of @a min or above.
 */
void VuoCompilerTargetSet::setMinMacVersion(enum MacVersion min)
{
	macVersionRange.first = min;
}

/**
 * Restricts the target set to Mac OS versions of @a max or below.
 */
void VuoCompilerTargetSet::setMaxMacVersion(enum MacVersion max)
{
	macVersionRange.second = max;
}

/**
 * Restricts the target set to the current (runtime) Mac OS version.
 */
void VuoCompilerTargetSet::restrictToCurrentOperatingSystemVersion(void)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	// http://stackoverflow.com/questions/11072804/mac-os-x-10-8-replacement-for-gestalt-for-testing-os-version-at-runtime
	SInt32 macMinorVersion;
	Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
#pragma clang diagnostic pop
	MacVersion macVersion = (MacVersion)(MacVersion_10_7 + (macMinorVersion - 7));
	macVersionRange.first = macVersion;
	macVersionRange.second = macVersion;
}

/**
 * Restricts the target set to the intersection between it and the other target set.
 */
void VuoCompilerTargetSet::restrictToBeCompatibleWithAllOf(const VuoCompilerTargetSet &other)
{
	if (! (other.macVersionRange.first == MacVersion_Any ||
		   (macVersionRange.first != MacVersion_Any && macVersionRange.first >= other.macVersionRange.first)) )
		macVersionRange.first = other.macVersionRange.first;

	if (! (other.macVersionRange.second == MacVersion_Any ||
		   (macVersionRange.second != MacVersion_Any && macVersionRange.second <= other.macVersionRange.second)) )
		macVersionRange.second = other.macVersionRange.second;
}

/**
 * Returns true if this target set is a superset of the other target set.
 */
bool VuoCompilerTargetSet::isCompatibleWithAllOf(const VuoCompilerTargetSet &other) const
{
	return ((macVersionRange.first == MacVersion_Any) ||
			((other.macVersionRange.first != MacVersion_Any) && (macVersionRange.first <= other.macVersionRange.first))) &&
			((macVersionRange.second == MacVersion_Any) ||
			 ((other.macVersionRange.first != MacVersion_Any) && (macVersionRange.second >= other.macVersionRange.second)));
}

/**
 * Returns a description of this target set.
 */
string VuoCompilerTargetSet::toString(void)
{
	if (macVersionRange.first == macVersionRange.second)
		return macVersionToString(macVersionRange.first);
	else if (MacVersion_Any == macVersionRange.first)
		return macVersionToString(macVersionRange.second) + " and below";
	else if (MacVersion_Any == macVersionRange.second)
		return macVersionToString(macVersionRange.first) + " and above";
	else
		return macVersionToString(macVersionRange.first) + " through " + macVersionToString(macVersionRange.second);
}

/**
 * Returns a string representation of the MacVersion.
 */
string VuoCompilerTargetSet::macVersionToString(MacVersion v)
{
	switch (v)
	{
		case MacVersion_Any:
			return "Mac OS X (all versions)";
		case MacVersion_10_7:
			return "Mac OS X 10.7";
		case MacVersion_10_8:
			return "Mac OS X 10.8";
		case MacVersion_10_9:
			return "Mac OS X 10.9";
	}
}
