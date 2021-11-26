function warnUnknown(type, elem)
	local ignore = {
		['\\newpage']     = true,
		['\\pagebreak']   = true,
		['\\nopagebreak'] = true,
		['\\hfill']       = true,
		['\\columnwidth'] = true,
	}
	if elem.format == 'tex' and not ignore[elem.text] then
		io.stderr:write(string.format("warning: unknown TeX %s tag: %s\n", type, elem.text))
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
