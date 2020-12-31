function warnUnknown(type, elem)
	local ignore = {
		['\\newpage']     = true,
		['\\pagebreak']   = true,
		['\\nopagebreak'] = true,
		['\\hfill']       = true,
		['\\columnwidth'] = true,
	}
	if elem.c[1] == 'tex' and not ignore[elem.c[2]] then
		io.stderr:write(string.format("warning: unknown TeX %s tag: %s\n", type, elem.c[2]))
	end
end

return {
	{RawBlock = function(elem)
		warnUnknown('block', elem)
	end},
	{RawInline = function(elem)
		warnUnknown('inline', elem)
	end},
}
