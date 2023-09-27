-- Convert TeX \term, \Term, and \definition tags into a new glossary section.

local glossary = {}

function makeIdentifier(term)
	term = string.lower(term)
	term = term:gsub('/[^a-z0-9 -]/', '')
	term = term:gsub(' ', '-')
	return term
end

function makeGlossaryIdentifier(term)
	return 'glossary-' .. makeIdentifier(term)
end

filters = {
	-- First, collect all the definitions into a term => definition table.
	{RawBlock = function(elem)
		if elem.format == 'tex' then
			foundAny = false
			for term, definition in elem.text:gmatch('\\definition{([^}]+)}{([^\n]+)}') do
				foundAny = true
				if glossary[term] then
					io.stderr:write(string.format('error: term "%s" is defined multiple times.\n', term))
				end
				glossary[term] = definition
			end
			if foundAny then
				return {}
			end
		end
	end},

	-- Then, make terms link to their definitions.
	{RawInline = function(elem)
		if elem.format == 'tex' then
			term = elem.text:gmatch('\\term{([^}]+)}')()
			if term then
				return pandoc.Link(
					pandoc.RawInline('html', '<phrase role="vuo-term">' .. term .. '</phrase>'),
					'#' .. makeGlossaryIdentifier(term))
			end

			-- \Term refers to a term beginning with a lowercase letter, but capitalizes just this current instance.
			term = elem.text:gmatch('\\Term{([^}]+)}')()
			if term then
				term = string.upper(term:sub(1, 1)) .. term:sub(2)
				return pandoc.Link(
					pandoc.RawInline('html', '<phrase role="vuo-term">' .. term .. '</phrase>'),
					'#' .. makeGlossaryIdentifier(term))
			end
		end
	end},

	-- Finally, append a new glossary section.
	{Pandoc = function(doc)
		table.insert(doc.blocks, pandoc.Header(1, pandoc.Str('Glossary'), {id='glossary'}))
		table.insert(doc.blocks, pandoc.Para({ pandoc.RawInline('html', '<abstract>Definitions of words Vuo uses</abstract>') }))
		table.insert(doc.blocks, pandoc.Para('Definitions of words Vuo uses.'))

		-- Sort case-insensitively.
		local orderedTerms = {}
		for term in pairs(glossary) do table.insert(orderedTerms, term) end
		table.sort(orderedTerms, function(a, b) return string.lower(a) < string.lower(b) end)

		for index, term in pairs(orderedTerms) do
			-- It would be nice to use the more-specific "DefinitionList",
			-- but I couldn't find a way to get it to emit an element "id"
			-- so I can create an anchor link to it.
			table.insert(doc.blocks, pandoc.Header(2, pandoc.Str(term), {id=makeGlossaryIdentifier(term)}))

			-- Parse Markdown in glossary definitions.
			definition = pandoc.read(glossary[term], 'markdown').blocks[1].content
			filteredDefinition = pandoc.walk_inline(pandoc.Span(definition), filters[2])

			-- Also produce a plaintext version of the glossary definition for the <abstract> tag.
			htmlDefinition = pandoc.write(pandoc.Pandoc(filteredDefinition), 'html')
			textDefinition = pandoc.write(pandoc.read(htmlDefinition, 'html'), 'plain')

			table.insert(doc.blocks, pandoc.Para({ pandoc.RawInline('html', '<abstract>' .. textDefinition .. '</abstract>') }))
			table.insert(doc.blocks, pandoc.Para(filteredDefinition))
		end


		-- Also generate a Mac Help Book Exact Match Searching database.
		local f = io.open('ExactMatch.plist', 'w')
		f:write([==[
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
]==])
		for index, term in pairs(orderedTerms) do
			key = string.lower(term)
			key = key:gsub('[ -]', '')
			f:write('<key>' .. key .. '</key>')
			f:write('<string>' .. makeGlossaryIdentifier(term) .. '</string>\n')
		end
		f:write('</dict></plist>')
		f:close()


		return doc
	end},
}

return filters
