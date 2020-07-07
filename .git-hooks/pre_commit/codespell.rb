module Overcommit::Hook::PreCommit
	class Codespell < Base
		def run
			extract_messages(
				execute(command, args: applicable_files).stdout.split("\n"),
				/^(?<file>[^:]+):((?<line>\d+):)?/,
			)
		end
	end
end
