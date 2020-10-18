/**
 * @file
 * VuoCompilerTargetSet implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerTargetSet.hh"

#include "VuoFileUtilitiesCocoa.hh"


/**
 * Creates a target set that is unrestricted. It includes any operating system version.
 */
VuoCompilerTargetSet::VuoCompilerTargetSet(void)
{
	macVersionRange = make_pair(MacVersion_Any, MacVersion_Any);
}

/**
 * Restricts the target set to macOS versions of @a min or above.
 */
void VuoCompilerTargetSet::setMinMacVersion(enum MacVersion min)
{
	macVersionRange.first = min;
}

/**
 * Returns a string (suitable for the `LSMinimumSystemVersion` Info.plist key) representing the minimum compatible OS version.
 *
 * If no minimum is specified, returns emptystring.
 */
string VuoCompilerTargetSet::getMinMacVersionString()
{
	switch (macVersionRange.first)
	{
		case MacVersion_Any:   return "";
		case MacVersion_10_11: return "10.11";
		case MacVersion_10_12: return "10.12";
		case MacVersion_10_13: return "10.13";
		case MacVersion_10_14: return "10.14";
		case MacVersion_10_15: return "10.15";
		case MacVersion_11_0:  return "11.0";
	}
}

/**
 * Restricts the target set to macOS versions of @a max or below.
 */
void VuoCompilerTargetSet::setMaxMacVersion(enum MacVersion max)
{
	macVersionRange.second = max;
}

/**
 * Restricts the target set to the current (runtime) macOS version.
 */
void VuoCompilerTargetSet::restrictToCurrentOperatingSystemVersion(void)
{
	int major = VuoFileUtilitiesCocoa_getOSVersionMajor();
	int minor = VuoFileUtilitiesCocoa_getOSVersionMinor();
	if (major == 10 && minor == 11)
		macVersionRange.first = macVersionRange.second = MacVersion_10_11;
	else if (major == 10 && minor == 12)
		macVersionRange.first = macVersionRange.second = MacVersion_10_12;
	else if (major == 10 && minor == 13)
		macVersionRange.first = macVersionRange.second = MacVersion_10_13;
	else if (major == 10 && minor == 14)
		macVersionRange.first = macVersionRange.second = MacVersion_10_14;
	else if (major == 10 && minor == 15)
		macVersionRange.first = macVersionRange.second = MacVersion_10_15;
	else if ((major == 10 && minor == 16) || (major == 11 && minor == 0))
		macVersionRange.first = macVersionRange.second = MacVersion_11_0;
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
			return "macOS (all versions)";
		case MacVersion_10_11:
			return "OS X 10.11";
		case MacVersion_10_12:
			return "macOS 10.12";
		case MacVersion_10_13:
			return "macOS 10.13";
		case MacVersion_10_14:
			return "macOS 10.14";
		case MacVersion_10_15:
			return "macOS 10.15";
		case MacVersion_11_0:
			return "macOS 11.0";
	}
}
