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

		// Highlight the current page.
		var links = tocContent.getElementsByTagName('a');
		for (var l in links)
			if (links[l].href == document.location.href)
				links[l].className = 'current';

		document.getElementById('toc').appendChild(tocContent);
	}

	var tocDest = document.getElementById('toc');
	// Use <object> instead of XMLHttpRequest or fetch so it works inside our Apple Help Book.
	// (CORS prevents using XMLHttpRequest and fetch on file:/// URIs, which Apple Help Books use.)
	tocDest.innerHTML='<object data="' + tocPageName + '" onload="updateTOC(this)" height="0"></object>';

	// Mark the titlepage for special formatting.
	if (document.getElementsByClassName('releaseinfo').length)
		document.getElementsByTagName('body')[0].className = 'titlepage';
}
