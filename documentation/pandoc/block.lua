-- Convert TeX tags into Pandoc block elements.

inlineFilter = require('inline')[1]

function texToDocbook(s)
	s = s:gsub('\\menu{([^}]+)}', '<phrase role="vuo-menu">%1</phrase>')
	s = s:gsub('\\emph{([^}]+)}', '<emphasis>%1</emphasis>')
	s = s:gsub('\\vuoNode{([^}]+)}', '<phrase role="vuo-node">%1</phrase>')
	s = s:gsub('\\vuoPort{([^}]+)}', '<phrase role="vuo-port">%1</phrase>')
	s = s:gsub('\\keys{([^}]+)}', '<shortcut>%1</shortcut>')
	s = s:gsub('\\code{([^}]+)}', '<code>%1</code>')
	s = s:gsub('\\hyperlink{([^}]+)}{([^}]+)}', '<link linkend="%1">%2</link>')
	s = s:gsub('\\doublequote', '"')
	s = s:gsub('\\textbackslash', '\\')
	s = s:gsub('\\textgreater', '&gt;')
	s = s:gsub('\\textasciitilde', '~')
	s = s:gsub('\\textunderscore', '_')
	s = s:gsub('\\%$', '$')
	s = s:gsub('\\ ', ' ')
	s = s:gsub('{}', '')
	s = s:gsub('https://[^ ]+', '<ulink url="%1">%1</ulink>')
	return s
end

function stringStartsWith(str, start)
   return str:sub(1, #start) == start
end

function replaceTag(blocks, elem, tag, prefix, suffix, matchOnlyAtBeginning)
	matchOnlyAtBeginning = matchOnlyAtBeginning or false
	for tagContent in elem.text:gmatch('\\' .. tag .. '{([^\n]+)}') do
		if matchOnlyAtBeginning and not stringStartsWith(elem.text, '\\' .. tag .. '{') then
			goto continue
		end

		tagContent = texToDocbook(tagContent)
		table.insert(blocks, pandoc.RawBlock('html', prefix .. tagContent .. suffix))

		if tagContent:find('\\[a-z]') then
			io.stderr:write(string.format("warning: unknown TeX tag in \\%s: %s\n", tag, tagContent))
		end

		::continue::
	end
end

return {
	{RawBlock = function(elem)
		if elem.format == 'tex' then
			local blocks = {}

			for language, caption, code in elem.text:gmatch('\\begin{lstlisting}%[language=([A-Za-z]+),caption=([^]]+)%](.+)\\end{lstlisting}') do
				table.insert(blocks, pandoc.RawBlock('docbook',
					'<example>'
						.. '<title>' .. caption .. '</title>'
						.. '<programlisting>' .. code .. '</programlisting>'
					.. '</example>'))
			end

			replaceTag(blocks, elem, 'tip', '<para role="vuo-tip">', '</para>')
			replaceTag(blocks, elem, 'noteTextProgrammers', '<para role="vuo-note-text">', '</para>')
			replaceTag(blocks, elem, 'menu', '<phrase role="vuo-menu">', '</phrase>', true)
			replaceTag(blocks, elem, 'code', '<programlisting>', '</programlisting>', true)

			if #blocks > 0 then
				return blocks
			end
		end
	end},
}
