require "open3"

module Overcommit::Hook::PreCommit
	class InvisibleUnicode < Base
		def run
			forbidden_unicode = {
				'\u0001' => 'start of heading (soh)',
				'\u0002' => 'start of text (stx)',
				'\u0003' => 'end of text (etx)',
				'\u0004' => 'end of transmission (eot)',
				'\u0005' => 'enquiry (enq)',
				'\u0006' => 'acknowledge (ack)',
				'\u0007' => 'bell (bel)',
				'\u0008' => 'backspace (bs)',
				'\u000b' => 'line tabulation (vt)',
				'\u000c' => 'form feed (ff)',
				'\u000e' => 'shift out (so)',
				'\u000f' => 'shift in (si)',
				'\u0010' => 'data link escape (dle)',
				'\u0011' => 'device control one (dc1)',
				'\u0012' => 'device control two (dc2)',
				'\u0013' => 'device control three (dc3)',
				'\u0014' => 'device control four (dc4)',
				'\u0015' => 'negative acknowledge (nak)',
				'\u0016' => 'synchronous idle (syn)',
				'\u0017' => 'end of transmission block (etb)',
				'\u0018' => 'cancel (can)',
				'\u0019' => 'end of medium (em)',
				'\u001a' => 'substitute (sub)',
				'\u001b' => 'escape (esc)',
				'\u001c' => 'file separator (fs)',
				'\u001d' => 'group separator (gs)',
				'\u001e' => 'record separator (rs)',
				'\u001f' => 'unit separator (us)',
				'\u007f' => 'delete (del)',
				'\u0080' => 'padding character (pad)',
				'\u0081' => 'high octet preset (hop)',
				'\u0082' => 'break permitted here (bph)',
				'\u0083' => 'no break here (nbh)',
				'\u0084' => 'index (ind)',
				'\u0085' => 'next line (nel)',
				'\u0086' => 'start of selected area (ssa)',
				'\u0087' => 'end of selected area (esa)',
				'\u0088' => 'horizontal tab set (hts)',
				'\u0089' => 'horizontal tab justified (htj)',
				'\u008a' => 'vertical tab set (vts)',
				'\u008b' => 'partial line forward (pld)',
				'\u008c' => 'partial line backward (plu)',
				'\u008d' => 'reverse line feed (ri)',
				'\u008e' => 'single-shift 2 (ss2)',
				'\u008f' => 'single-shift 3 (ss3)',
				'\u0090' => 'device control string (dcs)',
				'\u0091' => 'private use 1 (pu1)',
				'\u0092' => 'private use 2 (pu2)',
				'\u0093' => 'set transmit state (sts)',
				'\u0094' => 'cancel character (cch)',
				'\u0095' => 'message waiting (mw)',
				'\u0096' => 'start of protected area (spa)',
				'\u0097' => 'end of protected area (epa)',
				'\u0098' => 'start of string (sos)',
				'\u0099' => 'single graphic char intro (sgci)',
				'\u009a' => 'single char intro (sci)',
				'\u009b' => 'control sequence intro (csi)',
				'\u009c' => 'string terminator (st)',
				'\u009d' => 'os command (osc)',
				'\u009e' => 'private message (pm)',
				'\u009f' => 'app program command (apc)',
				'\u00ad' => 'soft hyphen',
				'\u061c' => 'Arabic letter mark',
				'\u200b' => 'zero width space',
				'\u200c' => 'zero width non-joiner (zwnj)',
				'\u200d' => 'zero width joiner (zwj)',
				'\u200e' => 'left-to-right mark',
				'\u200f' => 'right-to-left mark',
				'\u202a' => 'left-to-right embedding',
				'\u202b' => 'right-to-left embedding',
				'\u202c' => 'pop directional formatting',
				'\u202d' => 'left-to-right override',
				'\u202e' => 'right-to-left override',
				'\u2060' => 'word joiner',
				'\u2061' => 'function application',
				'\u2062' => 'invisible times',
				'\u2063' => 'invisible separator',
				'\u2064' => 'invisible plus',
				'\u2066' => 'left-to-right isolate',
				'\u2067' => 'right-to-left isolate',
				'\u2068' => 'first strong isolate',
				'\u2069' => 'pop directional isolate',
				'\u206a' => 'inhibit symmetric swapping',
				'\u206b' => 'activate symmetric swapping',
				'\u206c' => 'inhibit Arabic form shaping',
				'\u206d' => 'activate Arabic form shaping',
				'\u206e' => 'national digit shapes',
				'\u206f' => 'nominal digit shapes',
				'\ufeff' => 'zero width no-break space',  # Or 'byte-order mark (bom)' if it's at the beginning of a document.
				'\ufff9' => 'interlinear annotation anchor',
				'\ufffa' => 'interlinear annotation separator',
				'\ufffb' => 'interlinear annotation terminator',
			}

			messages = []
			applicable_files.each do |file|
				text_file?(file) or next

				IO.readlines(file).each_with_index do |line, line_number|
					line_number += 1
					line_annotated = line
					forbidden_unicode.each do |key, value|
						line_annotated = line_annotated.gsub(/#{key}/) { "\033[43;30m #{value} \033[0;1;31m" }
					end
					if line_annotated != line
						messages << Overcommit::Hook::Message.new(
							:error,
							file,
							line_number,
							"#{file}:#{line_number}  #{line_annotated}"
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
