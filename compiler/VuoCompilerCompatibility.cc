/**
 * @file
 * VuoCompilerCompatibility implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerCompatibility.hh"
#include "VuoStringUtilities.hh"

const map<string, string> VuoCompilerCompatibility::knownPlatforms = {
	{"macos", "macOS"}
};

const map<string, string> VuoCompilerCompatibility::knownArchitectures = {
	{"x86_64", "an Intel (X86-64) CPU"},
	{"arm64", "an Apple Silicon (ARM64/M1/M2/M3) CPU"}
};

/**
 * Returns a representation of a module's compatibility with a subset of Vuo's supported targets.
 *
 * @param json The "compatibility" value from the module's metadata.
 *
 * @see VuoModuleMetadata
 */
VuoCompilerCompatibility::VuoCompilerCompatibility(json_object *json)
{
	this->json = json;

	if (json)
		json_object_get(json);
}

/**
 * Copy constructor.
 */
VuoCompilerCompatibility::VuoCompilerCompatibility(const VuoCompilerCompatibility &other)
{
	this->json = other.json;

	if (json)
		json_object_get(json);
}

/**
 * Overloaded assignment operator.
 */
VuoCompilerCompatibility& VuoCompilerCompatibility::operator=(const VuoCompilerCompatibility &other)
{
	if (this == &other)
		return *this;

	this->json = other.json;

	if (json)
		json_object_get(json);

	return *this;
}

/**
 * Destructor.
 */
VuoCompilerCompatibility::~VuoCompilerCompatibility(void)
{
	if (json)
		json_object_put(json);
}

/**
 * Returns true if the targets of `this` are a superset of the targets of @a other.
 */
bool VuoCompilerCompatibility::isCompatibleWith(const VuoCompilerCompatibility &other)
{
	VuoCompilerCompatibility intersect = intersection(other);

	if (! intersect.json || ! other.json)
		return ! intersect.json && ! other.json;

	return json_object_equal(intersect.json, other.json);
}

/**
 * Returns the targets that `this` and @a other have in common.
 */
VuoCompilerCompatibility VuoCompilerCompatibility::intersection(const VuoCompilerCompatibility &other)
{
	if (! json)
		return other;

	if (! other.json)
		return *this;

	json_object *intersect = json_object_new_object();

	vector<string> platforms;
	{
		json_object_object_foreach(json, platform, val)
		{
			platforms.push_back(platform);
		}
	}
	{
		json_object_object_foreach(other.json, platform, val)
		{
			platforms.push_back(platform);
		}
	}

	for (string platform : platforms)
	{
		// Is this platform compatible with both?

		bool thisPlatformCompatible;
		json_object *thisPlatformVal = findPlatform(json, platform, thisPlatformCompatible);

		bool otherPlatformCompatible;
		json_object *otherPlatformVal = findPlatform(other.json, platform, otherPlatformCompatible);

		if (! (thisPlatformCompatible && otherPlatformCompatible))
		{
			json_object_object_add(intersect, platform.c_str(), json_object_new_boolean(false));
			continue;  // The platform is not compatible with both.
		}

		if (! otherPlatformVal)
		{
			json_object_object_add(intersect, platform.c_str(), thisPlatformVal);
			continue;  // The platform only has restrictions in `this`.
		}

		if (! thisPlatformVal)
		{
			json_object_object_add(intersect, platform.c_str(), otherPlatformVal);
			continue;  // The platform only has restrictions in `other`.
		}

		// Which architectures are compatible with both?

		vector<string> thisArchVec = findArchitectures(thisPlatformVal);
		vector<string> otherArchVec = findArchitectures(otherPlatformVal);

		vector<string> intersectArchVec;
		if (thisArchVec.empty())
			intersectArchVec = otherArchVec;
		else if (otherArchVec.empty())
			intersectArchVec = thisArchVec;
		else
		{
			set_intersection(thisArchVec.begin(), thisArchVec.end(),
							 otherArchVec.begin(), otherArchVec.end(),
							 std::back_inserter(intersectArchVec));

			if (intersectArchVec.empty())
			{
				json_object_object_add(intersect, platform.c_str(), json_object_new_boolean(false));
				continue;  // No architectures are compatible with both.
			}
		}

		json_object *intersectArch = nullptr;
		if (! intersectArchVec.empty())
		{
			intersectArch = json_object_new_array_ext(intersectArchVec.size());
			for (size_t i = 0; i < intersectArchVec.size(); ++i)
			{
				json_object *a = json_object_new_string(intersectArchVec[i].c_str());
				json_object_array_put_idx(intersectArch, i, a);
			}
		}

		// Which OS versions are compatible with both?

		string thisMin = findVersion(thisPlatformVal, "min");
		string thisMax = findVersion(thisPlatformVal, "max");
		string otherMin = findVersion(otherPlatformVal, "min");
		string otherMax = findVersion(otherPlatformVal, "max");

		auto compareVersions = [](const string &a, const string &b)
		{
			vector<string> as = VuoStringUtilities::split(a, '.');
			vector<string> bs = VuoStringUtilities::split(b, '.');
			for (size_t i = 0; i < max(as.size(), bs.size()); ++i)
			{
				int ai = (i < as.size() ? atoi(as[i].c_str()) : 0);
				int bi = (i < bs.size() ? atoi(bs[i].c_str()) : 0);
				if (ai < bi)
					return true;
				if (bi < ai)
					return false;
			}
			return false;
		};

		string greaterMin;
		if (! thisMin.empty() || ! otherMin.empty())
		{
			if (thisMin.empty())
				greaterMin = otherMin;
			else if (otherMin.empty())
				greaterMin = thisMin;
			else
			{
				vector<string> mins = {thisMin, otherMin};
				sort(mins.begin(), mins.end(), compareVersions);
				greaterMin = mins[1];
			}
		}

		string lesserMax;
		if (! thisMax.empty() || ! otherMax.empty())
		{
			if (thisMax.empty())
				lesserMax = otherMax;
			else if (otherMax.empty())
				lesserMax = thisMax;
			else
			{
				vector<string> maxes = {thisMax, otherMax};
				sort(maxes.begin(), maxes.end(), compareVersions);
				lesserMax = maxes[0];
			}
		}

		if (! greaterMin.empty() && ! lesserMax.empty())
		{
			vector<string> minMax = {greaterMin, lesserMax};
			vector<string> sortedMinMax = minMax;
			sort(sortedMinMax.begin(), sortedMinMax.end(), compareVersions);
			if (minMax != sortedMinMax)
			{
				json_object_object_add(intersect, platform.c_str(), json_object_new_boolean(false));
				continue;  // No OS versions are compatible with both.
			}
		}

		json_object *intersectMinVersion = nullptr;
		if (! greaterMin.empty())
			intersectMinVersion = json_object_new_string(greaterMin.c_str());

		json_object *intersectMaxVersion = nullptr;
		if (! lesserMax.empty())
			intersectMaxVersion = json_object_new_string(lesserMax.c_str());

		// Add the restrictions for this platform.

		if (intersectArch || intersectMinVersion || intersectMaxVersion)
		{
			json_object *intersectPlatform = json_object_new_object();
			if (intersectArch)
				json_object_object_add(intersectPlatform, "arch", intersectArch);
			if (intersectMinVersion)
				json_object_object_add(intersectPlatform, "min", intersectMinVersion);
			if (intersectMaxVersion)
				json_object_object_add(intersectPlatform, "max", intersectMaxVersion);
			json_object_object_add(intersect, platform.c_str(), intersectPlatform);
		}
	}

	return VuoCompilerCompatibility(intersect);
}

/**
 * Returns a human-readable description.
 */
string VuoCompilerCompatibility::toString(void)
{
	if (! json)
		return "any system that Vuo supports";

	auto labelForKey = [](const string &key, const map<string, string> &keysAndLabels)
	{
		auto iter = keysAndLabels.find(key);
		if (iter != keysAndLabels.end())
			return iter->second;
		else
			return key;
	};

	auto macOSDisplayName = [](const string &version) {
		if (version < "10.16" || version >= "11")
			return version;

		// Map "10.16" to "11", and "10.17" to "12", …
		auto parts = VuoStringUtilities::split(version, '.');
		if (parts.size() < 2)
			return version;

		return to_string(stoi(parts[1]) - 5);
	};

	vector<string> platformStrings;
	json_object_object_foreach(json, platformKey, platformVal)
	{
		// Platform

		string p = labelForKey(platformKey, knownPlatforms);

		// OS versions

		string min = findVersion(platformVal, "min");
		string max = findVersion(platformVal, "max");

		string versions;
		if (! min.empty() || ! max.empty())
		{
			if (! min.empty())
				min = macOSDisplayName(min);
			if (! max.empty())
				max = macOSDisplayName(max);

			if (min == max)
				versions = min;
			else if (min.empty())
				versions = max + " and below";
			else if (max.empty())
				versions = min + " and above";
			else
				versions = min + " through " + max;
		}

		// Architectures

		vector<string> archKeys = findArchitectures(platformVal);
		vector<string> archList;
		std::transform(archKeys.begin(), archKeys.end(),
					   std::back_inserter(archList),
					   [&](string a) { return labelForKey(a, knownArchitectures); });
		string architectures = VuoStringUtilities::join(archList, " or ");

		if (! versions.empty())
			p += " " + versions;
		if (! architectures.empty())
			p += " on " + architectures;
		platformStrings.push_back(p);
	}

	return VuoStringUtilities::join(platformStrings, ", or ");
}

/**
 * Returns a JSON-formatted string as it would appear in `VuoModuleMetadata`.
 */
string VuoCompilerCompatibility::toJsonString(void)
{
	return json_object_to_json_string_ext(json, JSON_C_TO_STRING_PLAIN);
}

/**
 * Returns true if at least one OS version and architecture is compatible on @a platform.
 */
bool VuoCompilerCompatibility::isCompatibleWithPlatform(const string &platform)
{
	bool isPlatformCompatible;
	findPlatform(json, platform.c_str(), isPlatformCompatible);
	return isPlatformCompatible;
}

/**
 * If only OS versions X and up are compatible on @a platform, returns X. Otherwise, returns an empty string.
 */
string VuoCompilerCompatibility::getMinVersionOnPlatform(const string &platform)
{
	bool isPlatformCompatible;
	json_object *platformVal = findPlatform(json, platform.c_str(), isPlatformCompatible);
	if (platformVal)
	{
		json_object *min = nullptr;
		if (json_object_object_get_ex(platformVal, "min", &min))
			return json_object_get_string(min);
	}

	return "";
}

/**
 * Returns a representation of the single platform, OS version, and architecture described by
 * the LLVM target triple @a target.
 */
VuoCompilerCompatibility VuoCompilerCompatibility::compatibilityWithTargetTriple(const string &target)
{
	vector<string> parts = VuoStringUtilities::split(target, '-');

	string arch = parts.at(0);

	string platform;
	string version;
	if (VuoStringUtilities::beginsWith(parts.at(2), "macosx"))
	{
		platform = "macos";

		version = parts.at(2).substr(6);
		version = version.substr(0, version.rfind("."));
	}

	json_object *json = json_object_new_object();

	json_object *platformVal = json_object_new_object();
	json_object_object_add(json, platform.c_str(), platformVal);

	json_object *archVal = json_object_new_string(arch.c_str());
	json_object *archArray = json_object_new_array_ext(1);
	json_object_array_put_idx(archArray, 0, archVal);
	json_object_object_add(platformVal, "arch", archArray);

	json_object *minVal = json_object_new_string(version.c_str());
	json_object *maxVal = json_object_new_string(version.c_str());
	json_object_object_add(platformVal, "min", minVal);
	json_object_object_add(platformVal, "max", maxVal);

	return VuoCompilerCompatibility(json);
}

/**
 * Returns a representation of compatibility with any target that Vuo supports.
 */
VuoCompilerCompatibility VuoCompilerCompatibility::compatibilityWithAnySystem(void)
{
	return VuoCompilerCompatibility(nullptr);
}

/**
 * Returns a representation of compatibility with @a architectures on each platform that Vuo supports.
 */
VuoCompilerCompatibility VuoCompilerCompatibility::compatibilityWithArchitectures(const set<string> &architectures)
{
	if (architectures.empty())
		return VuoCompilerCompatibility(nullptr);

	set<string> knownArchitectureKeys;
	std::transform(knownArchitectures.begin(), knownArchitectures.end(),
				   std::inserter(knownArchitectureKeys, knownArchitectureKeys.end()),
				   [](const pair<string, string> &p) { return p.first; });
	if (knownArchitectureKeys == architectures)
		return VuoCompilerCompatibility(nullptr);

	json_object *archArray = json_object_new_array_ext(architectures.size());
	int i = 0;
	for (string arch : architectures)
	{
		json_object *archVal = json_object_new_string(arch.c_str());
		json_object_array_put_idx(archArray, i++, archVal);
	}

	json_object *json = json_object_new_object();

	for (auto platform : knownPlatforms)
	{
		json_object *platformVal = json_object_new_object();
		json_object_object_add(json, platform.first.c_str(), platformVal);

		json_object_object_add(platformVal, "arch", archArray);
		json_object_get(archArray);
	}

	json_object_put(archArray);

	return VuoCompilerCompatibility(json);
}

json_object * VuoCompilerCompatibility::findPlatform(json_object *json, string platformKey, bool &isPlatformCompatible)
{
	isPlatformCompatible = true;

	json_object *platformVal = nullptr;
	if (json_object_object_get_ex(json, platformKey.c_str(), &platformVal))
	{
		if (json_object_is_type(platformVal, json_type_boolean))
			isPlatformCompatible = false;

		return platformVal;
	}

	return nullptr;
}

string VuoCompilerCompatibility::findVersion(json_object *platformVal, const string &minOrMax)
{
	json_object *versionVal;
	if (json_object_object_get_ex(platformVal, minOrMax.c_str(), &versionVal))
		return json_object_get_string(versionVal);

	return "";
}

vector<string> VuoCompilerCompatibility::findArchitectures(json_object *platformVal)
{
	vector<string> architectures;

	json_object *arch = nullptr;
	if (json_object_object_get_ex(platformVal, "arch", &arch))
	{
		size_t len = json_object_array_length(arch);
		for (size_t i = 0; i < len; ++i)
		{
			json_object *a = json_object_array_get_idx(arch, i);
			architectures.push_back(json_object_get_string(a));
		}
	}

	return architectures;
}
