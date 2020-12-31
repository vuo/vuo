-- Convert TeX tags into Pandoc inline elements.

function texToDocbook(s)
	s = s:gsub('\\&', '&amp;')
	return s
end

return {
	{RawInline = function(elem)
		if elem.c[1] == 'tex' then
			code = elem.c[2]:gmatch('\\code{([^}]+)}')()
			if code then
				return pandoc.Code(code)
			end

			emph = elem.c[2]:gmatch('\\emph{([^}]+)}')()
			if emph then
				return pandoc.Emph(emph)
			end

			node = elem.c[2]:gmatch('\\vuoNode{([^}]+)}')()
			if node then
				-- The Span element's class doesn't make it into the DocBook output, but RawInline does.
				-- return pandoc.Span(node, {class='vuo-node'})
				return pandoc.RawInline('html', '<phrase role="vuo-node">' .. node .. '</phrase>')
			end

			nodeClass = elem.c[2]:gmatch('\\vuoNodeClass{([^}]+)}')()
			if nodeClass then
				return pandoc.RawInline('html', '<phrase role="vuo-node-class">' .. nodeClass .. '</phrase>')
			end

			port = elem.c[2]:gmatch('\\vuoPort{([^}]+)}')()
			if port then
				return pandoc.RawInline('html', '<phrase role="vuo-port">' .. port .. '</phrase>')
			end

			keys = elem.c[2]:gmatch('\\keys{([^}]+)}')()
			if keys then
				return pandoc.RawInline('html', '<shortcut>' .. keys .. '</shortcut>')
			end

			menu = elem.c[2]:gmatch('\\menu{([^}]+)}')()
			if menu then
				return pandoc.RawInline('html', '<phrase role="vuo-menu">' .. texToDocbook(menu) .. '</phrase>')
			end

			directory = elem.c[2]:gmatch('\\directory{([^}]+)}')()
			if directory then
				return pandoc.RawInline('html', '<phrase role="vuo-directory">' .. directory .. '</phrase>')
			end
		end
	end},
}
