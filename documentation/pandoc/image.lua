-- Convert TeX tags into Pandoc `Image` elements.

return {
	{RawBlock = function(elem)
		if elem.format == 'tex' then
			local images = {}

			for name in elem.text:gmatch('\\vuoScreenshot{([^}]+)}') do
				table.insert(images, pandoc.Image({}, string.format('image/%s.png', name)))
			end

			for name in elem.text:gmatch('\\vuoCompositionImage{([^}]+)}') do
				table.insert(images, pandoc.Image({}, string.format('image-generated/%s.png', name)))
			end

			for name in elem.text:gmatch('\\vuoNodeClassImage{([^}]+)}') do
				table.insert(images, pandoc.Image({}, string.format('image-generated/%s.png', name)))
			end

			for size, name in elem.text:gmatch('\\vuoImage{([0-9.]+)}{([^}]+).png}') do
				table.insert(images, pandoc.Image({}, string.format('image/%s.png', name)))
			end

			for size, name in elem.text:gmatch('\\vuoImage{([0-9.]+)}{([^}]+).pdf}') do
				table.insert(images, pandoc.Image({}, string.format('image-generated/%s.png', name)))
			end

			if #images > 0 then
				return pandoc.Para(images)
			end
		end
	end},
}
