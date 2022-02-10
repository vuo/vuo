if (VUO_ANALYZE)
	# Runs clang-analyzer, oclint, and infer.
	set(CMAKE_C_COMPILER_LAUNCHER   ${PROJECT_SOURCE_DIR}/base/build-and-analyze ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
	set(CMAKE_CXX_COMPILER_LAUNCHER ${PROJECT_SOURCE_DIR}/base/build-and-analyze ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})

	add_custom_target(VuoAnalyze
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}

		COMMAND ${CMAKE_COMMAND} -E env PATH="/opt/homebrew/opt/grep/libexec/gnubin:/usr/local/opt/grep/libexec/gnubin:/opt/homebrew/opt/check-all-the-things:/usr/local/opt/check-all-the-things:/Library/TeX/texbin:$ENV{PATH}"
			check-all-the-things
				--jobs 9
				--checks-output-lines 50
				# Jenkinsfile runs cppcheck, so no need to run it again here.
				# Disable other unneeded/non-working/not-available-on-macOS checks.
				--checks '- cppcheck deheader include-what-you-use embed-readme empty fontlint pydocstyle pmccabe clang-tidy clang-check clamav complexity doc8 dodgy pscan spellintian suspicious-source licensecheck licensecheck-generated-files licensecheck-incorrect-fsf-address codespell anorack kwstyle splint opencolladavalidator mp3check mp3val checkmp3 bashate busybox-sh-syntax-check font-embedding-restrictions csslint-libcroco dash-syntax-check ftlint ftvalid closure-linter jsonlint-php jsonlint-py3 lintex perlcritic php7cc podlint pyflakes pyflakes3 pylint pylint3 python2-bandit python3-bandit roodi rubocop vulture'
			| grep -E -v
				-e '^$$'
				-e '^\.\.\.$$'
				-e '\(miniz|shared_mutex|VuoUrlParser\).\(c|cpp|h|hh\)'
				-e '\.t2d:'
				-e '^\# This command needs'
				-e 'Flawfinder version'
				-e 'Number of rules'
				-e 'CMakeLists.txt:.*: typography.symbols.curly_quotes'
				-e 'could not find any possible bashisms in bash script'
				-e '\.git/'
				-e 'alias:.*invalid UTF-8 code'
				-e 'Remarks:'
				-e 'cmdline disabled check:'
				-e 'dangerous check:'
				-e 'help needed:'
				-e 'modifies files:'
				-e 'no matching files:'
				-e 'no output:'
				-e 'no specific file name checks:'
				-e 'no specific file name wildcard checks:'
				-e 'trimmed:'
				-e 'ANALYSIS SUMMARY:'
				-e 'No hits found.'
				-e 'Lines analyzed = '
				-e 'Physical Source Lines of Code'
				-e 'Hits@level'
				-e 'Hits/KSLOC@level'
				-e 'Minimum risk level = '
				-e 'There may be other security vulnerabilities'
				-e 'Secure Programming HOWTO'
				-e 'dwheeler.com/secure-programs'
	)
endif()
