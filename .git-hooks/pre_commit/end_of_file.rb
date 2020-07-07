module Overcommit::Hook::PreCommit
	class EndOfFile < Base
		def run
			messages = []
			applicable_files.each do |file|
				text_file?(file) or next

				f = File.new(file)

				f.seek(-1, IO::SEEK_END)
				if f.readbyte != 10
					messages << Overcommit::Hook::Message.new(:error, file, nil, "#{file}: Missing newline at end of file")
					next
				end

				f.seek(-2, IO::SEEK_END)
				if f.readbyte == 10
					messages << Overcommit::Hook::Message.new(:error, file, nil, "#{file}: Multiple newlines at end of file")
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
