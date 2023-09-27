option(VUO_CONTRIBUTOR_LIST "Whether to query the full list of contributors from community.vuo.org." OFF)

# Generates a list of Vuo contributors and dependencies, rendered in a few formats.
function (VuoContributorList)
	set(contributorsFile       "${CMAKE_BINARY_DIR}/documentation/CONTRIBUTORS-raw.md")
	set(contributorsTextFile   "${CMAKE_BINARY_DIR}/documentation/CONTRIBUTORS.md")
	set(contributorsPandocFile "${CMAKE_BINARY_DIR}/documentation/CONTRIBUTORS-pandoc.md")
	set(dependenciesPandocFile "${CMAKE_BINARY_DIR}/documentation/DEPENDENCIES-pandoc.md")
	if (EXISTS ${contributorsFile})
		return()
	endif()

	if (VUO_CONTRIBUTOR_LIST)
		message(STATUS "Fetching contributor list…")

		set(contributorBadgeIDs
			6    # "Nice Reply"
			18   # "Nice Topic"
			36   # "Appreciated"
			38   # "Thank You"
			102  # "Composition Sharer"
			103  # "Node Sharer"
			104  # "Bug Finder"
			105  # "Feature Inspirer"
			108  # "Translator"
			109  # "Spotlight"
			110  # "Donor"
			111  # "Founder"
			112  # "Prolific Composition Sharer"
			113  # "Prolific Node Sharer"
			115  # "Code Contributor"
			116  # "Commissioner"
			117  # "Creator"
			119  # "Tutorial Author"
			120  # "Presenter"
			122  # "Bug Collector"
			123  # "Feature Influencer"
			124  # "Feature Advocate"
			125  # "Feature Champion"
			126  # "Patron"
		)
		set(contributorLines "")
		foreach (contributorBadgeID ${contributorBadgeIDs})
			set(offset 0)
			while (${offset} GREATER_EQUAL 0)
				VuoContributorListFetch(fetchedContributorLines offset ${contributorBadgeID} ${offset})
				list(APPEND contributorLines ${fetchedContributorLines})
			endwhile()
		endforeach()
		list(SORT contributorLines CASE INSENSITIVE)
		list(REMOVE_DUPLICATES contributorLines)
		list(JOIN contributorLines "\n" contributorListText)

		message(STATUS "    Done.")
	else()
		set(contributorListText "(quick build; omitted contributor list)")
	endif()

	file(WRITE ${contributorsFile} ${contributorListText})

	configure_file(${PROJECT_SOURCE_DIR}/documentation/contributors-template-text.md   ${contributorsTextFile})
	configure_file(${PROJECT_SOURCE_DIR}/documentation/contributors-template-pandoc.md ${contributorsPandocFile})

	file(READ ${CMAKE_SOURCE_DIR}/DEPENDENCIES.md dependenciesListText)
	configure_file(${PROJECT_SOURCE_DIR}/documentation/dependencies-template-pandoc.md ${dependenciesPandocFile})
endfunction()

# Performs one Discourse REST API request for `badgeID` at `offset`,
# and places the result in `contributorLines`.
# Sets `nextOffset` to a nonnegative value if the caller should invoke this function again.
function (VuoContributorListFetch contributorLines nextOffset badgeID offset)
	set(badgeInfoURL "https://community.vuo.org/user_badges.json?badge_id=${badgeID}&offset=${offset}")
	message(STATUS "    ${badgeInfoURL}")
	execute_process(
		COMMAND /usr/bin/curl -s ${badgeInfoURL}
		TIMEOUT 30
		RESULT_VARIABLE status
		ERROR_VARIABLE error
		OUTPUT_VARIABLE badgeInfoJSON
	)
	if (NOT status EQUAL 0)
		message(FATAL_ERROR "`curl ${badgeInfoURL}` failed: ${error}")
	endif()

	# Avoid triggering Discourse's rate-limiter.
	execute_process(COMMAND /bin/sleep 0.25)

	string(JSON userBadgesCount LENGTH ${badgeInfoJSON} user_badge_info user_badges)
	if (userBadgesCount EQUAL 0)
		# We've reached the end of the paged query.
		set(${contributorLines} "" PARENT_SCOPE)
		set(${nextOffset} -1 PARENT_SCOPE)
		return()
	endif()

	string(JSON usersCount LENGTH ${badgeInfoJSON} users)
	set(i 0)
	set(_contributorLines "")
	set(excludedUsernames root system)
	while (i LESS ${usersCount})
		string(JSON username GET ${badgeInfoJSON} users ${i} username)
		if (NOT username IN_LIST excludedUsernames)
			list(APPEND _contributorLines "   - [${username}](https://community.vuo.org/u/${username})")
		endif()
		math(EXPR i "${i} + 1")
	endwhile()

	set(${contributorLines} ${_contributorLines} PARENT_SCOPE)

	string(JSON grantCount GET ${badgeInfoJSON} badges 0 grant_count)
	math(EXPR _nextOffset "${offset} + ${userBadgesCount}")
	if (_nextOffset GREATER_EQUAL grantCount)
		set(${nextOffset} -1 PARENT_SCOPE)
	else()
		set(${nextOffset} ${_nextOffset} PARENT_SCOPE)
	endif ()
endfunction()
