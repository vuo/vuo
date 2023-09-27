var pathComponents = document.location.pathname.split('/');
var pageName = pathComponents[pathComponents.length - 1];

// Figure out whether we're using .xhtml or .html page extensions.
// (doc.vuo.org uses the former; Apple Help Book uses the latter.)
var tocPageName = 'ar01-toc.xhtml';
if (pageName.substring(pageName.length - 5) == ".html")
	tocPageName = 'ar01-toc.html';

if (pageName != tocPageName) {
	// Extract the Table of Contents from the <object> element, and display it as a sidebar.
	// Workaround for docbook-xsl's inability to bake the TOC into each chunked page.
	function updateTOC(o) {
		var tocContent = o.contentDocument.getElementsByClassName('toc')[0];

		// Find menu items having child menu items…
		var itemsToTransform = [];
		const items = tocContent.getElementsByTagName('li');
		for (const item of items) {
			if (item.tagName != 'li')
				continue;
			if (item.getElementsByTagName('ul').length == 0)
				continue;

			itemsToTransform.push(item.querySelector('span > a').attributes.href.value);
		}

		// …and convert each into a disclosure (details/summary) widget.
		for (var itemHref of itemsToTransform) {
			var linkItem = tocContent.querySelector('a[href="' + itemHref + '"]');
			var spanItem = linkItem.parentElement;
			var liItem = spanItem.parentElement;

			var details = document.createElement('details');
			while (liItem.firstChild)
				details.appendChild(liItem.firstChild);
			liItem.replaceWith(details);

			var summary = document.createElement('summary');
			while (spanItem.firstChild)
				summary.appendChild(spanItem.firstChild);
			spanItem.replaceWith(summary);
		}

		// Highlight the current page and the path leading up to it, and open the disclosure widgets.
		var linkItems = tocContent.getElementsByTagName('a');
		for (var linkItem of linkItems) {
			if (linkItem.href != document.location.href)
				continue;

			if (linkItem.parentElement.tagName == 'summary') {
				var summaryItem = linkItem.parentElement;
				var detailsItem = summaryItem.parentElement;
				while (detailsItem) {
					detailsItem.open = true;
					detailsItem.querySelector('summary > a').className = 'active-trail';

					var ulItem = detailsItem.parentElement;
					if (ulItem.tagName != 'ul' || ulItem.className == 'toc')
						break;

					detailsItem = ulItem.parentElement;
				}
			}
			else if (linkItem.parentElement.tagName == 'span') {
				var spanItem = linkItem.parentElement;
				var liItem = spanItem.parentElement;
				var ulItem = liItem.parentElement;
				while (ulItem) {
					var detailsItem = ulItem.parentElement;
					if (detailsItem.tagName != 'details')
						break;

					detailsItem.open = true;
					detailsItem.querySelector('summary > a').className = 'active-trail';

					ulItem = detailsItem.parentElement;
				}
			}

			linkItem.className = 'current';

			break;
		}

		// Add a search widget.
		var tocHierarchicalList = tocContent.getElementsByTagName('ul')[0];
		var searchFieldResults = document.createElement('ul');
		searchFieldResults.className = 'search-results';
		var searchField = document.createElement('input');
		searchField.type = 'search';
		searchField.placeholder = 'Search section titles…';
		searchField.addEventListener('keyup', (event) => {
			searchFieldResults.replaceChildren();

			var searchText = event.target.value.toLowerCase();
			if (searchText.length) {
				tocHierarchicalList.style.display = 'none';

				const links = tocHierarchicalList.getElementsByTagName('a');
				for (const link of links) {
					if (link.textContent.toLowerCase().includes(searchText)) {
						var result = document.createElement('li');
						result.appendChild(link.cloneNode(true));
						searchFieldResults.appendChild(result);

						if (searchFieldResults.children.length >= 20)
							break;
					}
				}
			}
			else
				tocHierarchicalList.style.display = 'inherit';
		});
		tocContent.insertBefore(searchField, tocHierarchicalList);
		tocContent.insertBefore(searchFieldResults, tocHierarchicalList);

		document.getElementById('toc').appendChild(tocContent);
	}

	var tocDest = document.getElementById('toc');
	// Use <object> instead of XMLHttpRequest or fetch so it works inside our Apple Help Book.
	// (CORS prevents using XMLHttpRequest and fetch on file:/// URIs, which Apple Help Books use.)
	tocDest.innerHTML='<object data="' + tocPageName + '" onload="updateTOC(this)" height="0"></object>';
}
