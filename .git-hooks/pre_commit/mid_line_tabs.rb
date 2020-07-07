module Overcommit::Hook::PreCommit
	class MidLineTabs < Base
		def run
			messages = []
			applicable_files.each do |file|
				text_file?(file) or next

				IO.readlines(file).each_with_index do |line, line_number|
					line_number += 1
					if /^([\t ]*[^\t ].*?)(\t.*)$/ =~ line
						line = line.gsub(/\t/) { "\033[43;30m tab \033[0;1;31m" }
						messages << Overcommit::Hook::Message.new(
							:error,
							file,
							line_number,
							"#{file}:#{line_number}  #{line}"
						)
					end
				end
			end
			messages
		end

		private
		def text_file?(filename)
		  file_type, status = Open3.capture2e("file", filename)
		  status.success? && file_type.include?("text")
		end
	end
end
