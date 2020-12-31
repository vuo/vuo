-- Convert TeX tags into Pandoc elements.
-- This is separate from `block.lua` and `inline.lua` since asides can be either block or inline,
-- and because inline asides need to be moved outside the paragraph they're contained in
-- (else Pandoc splits the paragraph).

return {
	-- First, promote all inline asides to blocks next to the paragraph they were in.
	{Blocks = function(blocks)
		for blockIndex, block in pairs(blocks) do
			if block.content then
				local asides = {}
				for inlineIndex, inline in pairs(block.content) do
					if inline.format == 'tex' then
						version = inline.text:gmatch('\\newInVersion{([0-9.]+)}')()
						if version then
							table.insert(asides, inline.text)
							block.content[inlineIndex] = pandoc.RawInline('html', '')
						end
					end
				end
				for asideIndex, aside in pairs(asides) do
					table.insert(blocks, blockIndex, pandoc.RawBlock('tex', aside))
				end
			end
		end

		return blocks
	end},

	-- Then, convert block asides to HTML.
	{RawBlock = function(elem)
		if elem.c[1] == 'tex' then
			local blocks = {}

			for version in elem.c[2]:gmatch('\\newInVersion{([0-9.]+)}') do
				table.insert(blocks, pandoc.RawBlock('html', '<para role="vuo-note-new"><phrase role="title">New in Vuo ' .. version .. '</phrase></para>'))
			end

			for version, changed in elem.c[2]:gmatch('\\changedInVersion{([0-9.]+)}{([^\n]+)}') do
				table.insert(blocks, pandoc.RawBlock('html', '<para role="vuo-note-changed"><phrase role="title">Changed in Vuo ' .. version .. '</phrase>' .. changed .. '</para>'))
			end

			if #blocks > 0 then
				return blocks
			end
		end
	end},
}
