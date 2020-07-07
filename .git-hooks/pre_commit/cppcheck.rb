module Overcommit::Hook::PreCommit
	class Cppcheck < Base
		def run
			extract_messages(
				execute(command, args: applicable_files).stderr.split("\n").select {|v|
					v !~ /(Unmatched suppression|Cppcheck cannot find all the include files|syntax error)/
				},
				# For example:
				# [path/file.hh:18]: (style) Class 'SomeClass' has a constructor with 1 argument that is not explicit.
				# [path/file.hh:27] -> [path2/file2.hh:146]: (style) The function 'key' overrides a function in a base class but is not marked with a 'override' specifier.
				/^(\[[^:]+:\d+\] -> )?\[(?<file>[^:]+):(?<line>\d+)\]:/,
			)
		end
	end
end
