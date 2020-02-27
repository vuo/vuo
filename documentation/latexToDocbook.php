#!/usr/bin/env php
<?php

function makeIdentifier($s)
{
    return str_replace(' ', '-', preg_replace('/[^a-z0-9 -]/', '', strtolower($s)));
}

function makeGlossaryIdentifier($s)
{
    return 'glossary-' . makeIdentifier($s);
}

// Converts TeX to DocBook XML (not HTML, despite the name).
function convertTexStringToHtml($s, $insideCode = false)
{
	$texToHtml = array(
		'\doublequote' => '"',
		'\textbackslash' => '\\',
		'\textgreater' => $insideCode ? '>' : '&gt;',
		'\textasciitilde' => '~',
		'\textunderscore' => '_',
		'\$' => '$',
		'\&' => '&amp;',
		'\ ' => ' ',
		'``' => $insideCode ? '“' : '&ldquo;',
		"''" => $insideCode ? '”' : '&rdquo;',
		'<' => $insideCode ? '<' : '&lt;',
		'>' => $insideCode ? '>' : '&gt;',
		' --- ' => $insideCode ? ' — ' : ' &mdash; '
	);
	foreach ($texToHtml as $tex => $html)
	{
		$s = str_replace($tex . '{}', $html, $s);
		$s = str_replace($tex, $html, $s);
	}
	$s = preg_replace('/\\\\vuoPort{([^}]+)}/', '<phrase role="vuo-port">$1</phrase>', $s);
	$s = preg_replace('/\\\\vuoNode{([^}]+)}/', '<phrase role="vuo-node">$1</phrase>', $s);
	$s = preg_replace('/\\\\menu{([^}]+)}/', '<phrase role="vuo-menu">$1</phrase>', $s);
	$s = preg_replace('/\\\\term{([^}]+)}/', '<phrase role="vuo-term">$1</phrase>', $s);
	$s = preg_replace('/\\\\code{([^}]+)}/', '<literal>$1</literal>', $s);
	$s = preg_replace('/\\\\keys{([^}]+)}/', '<shortcut>$1</shortcut>', $s);
	$s = preg_replace('/\\\\emph{([^}]+)}/', '<emphasis>$1</emphasis>', $s);

	// Convert plaintext URLs into links.
	$s = preg_replace('|(https?://[^/]+/[^ ]+)|', '<ulink url="$1">$1</ulink>', $s);

	// Special case for Keyboard Shortcuts table
	$s = preg_replace('/\\\\hbox{\\\\strut ([^}]+)}\\\\hbox{\\\\strut ([^}]+)/', "<literallayout>$1\n$2</literallayout>", $s);

	return $s;
}

function processJSON(&$book)
{
	global $glossary;
	foreach ($book as $k => &$v)
	{
		// Convert TeX commands into Docbook commands.
		if (is_array($v)
			&& !empty($v['RawInline'][0])
			&& $v['RawInline'][0] === 'tex')
		{
			$tex = $v['RawInline'][1];

			if (strpos($tex, '\vuoScreenshot{') === 0)
			{
				$image = rtrim(substr($tex, strlen('\vuoScreenshot{')), '}');
				$v = array('Image' => array(array(), array("image/$image.png", '')));
			}
			else if (strpos($tex, '\vuoCompositionImage{') === 0)
			{
				$image = rtrim(substr($tex, strlen('\vuoCompositionImage{')), '}');
				$v = array('Image' => array(array(), array("image-generated/$image.png", '')));
			}
			else if (strpos($tex, '\vuoNodeClassImage{') === 0)
			{
				$image = rtrim(substr($tex, strlen('\vuoNodeClassImage{')), '}');
				$v = array('Image' => array(array(), array("image-generated/$image.png", '')));
			}
			else if (preg_match('/\\\vuoImage{([0-9.]+)}{([^}]+\.png)}/', $tex, $matches))
				$v = array('Image' => array(array(), array('image/' . $matches[2], '')));
			else if (preg_match('/\\\vuoImage{([0-9.]+)}{([^}]+)\.pdf}/', $tex, $matches))
				$v = array('Image' => array(array(), array('image-generated/' . $matches[2] . '.png', '')));
			else if (strpos($tex, '\code{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\code{')), '}');
				$v = array('Code' => array(array("",array(),array()), convertTexStringToHtml($text, true)));
			}
			else if (strpos($tex, '\vuoNode{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\vuoNode{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-node">' . convertTexStringToHtml($text) . '</phrase>'));
			}
			else if (strpos($tex, '\vuoNodeClass{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\vuoNodeClass{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-node-class">' . convertTexStringToHtml($text) . '</phrase>'));
			}
			else if (strpos($tex, '\vuoPort{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\vuoPort{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-port">' . convertTexStringToHtml($text) . '</phrase>'));
			}
			else if (strpos($tex, '\tip{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\tip{')), '}');
				$v = array('RawInline' => array('html', '<para role="vuo-tip">' . convertTexStringToHtml($text) . '</para>'));
			}
			else if (strpos($tex, '\noteTextProgrammers{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\noteTextProgrammers{')), '}');
				$v = array('RawInline' => array('html', '<para role="vuo-note-text">' . convertTexStringToHtml($text) . '</para>'));
			}
			else if (strpos($tex, '\newInVersion{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\newInVersion{')), '}');
				// Move the newInVersion tag to the beginning of this paragraph.
				$v = $book[0];
				$book[0] = array('RawInline' => array('html', '<para role="vuo-note-new"><phrase role="title">New in Vuo ' . convertTexStringToHtml($text) . '</phrase></para>'));
			}
			else if (preg_match('/\\\changedInVersion{([^}]+)}{([^}]+)}/', $tex, $matches))
				$v = array('RawInline' => array('html', '<para role="vuo-note-changed"><phrase role="title">Changed in Vuo ' . convertTexStringToHtml($matches[1]) . '</phrase>' . convertTexStringToHtml($matches[2]) . '</para>'));
			else if (strpos($tex, '\term{') === 0
				||   strpos($tex, '\Term{') === 0)
			{
				$capitalize = ($tex[1] === 'T');
				$text = rtrim(substr($tex, strlen('\term{')), '}');
				$v = [ "Link" => [
					[
						// [ 'Str' => $text ]
						[ 'RawInline' => [ 'html', '<phrase role="vuo-term">' . convertTexStringToHtml($text) . '</phrase>' ] ]
					],
					[
						'#' . makeGlossaryIdentifier($text),
						''
					]
				]];
			}
			else if (strpos($tex, '\definition{') === 0)
			{
				preg_match('/^.definition{(.*)}{(.*)}$/', $tex, $matches);
				$glossary[$matches[1]] = $matches[2];
			}
			else if (strpos($tex, '\keys{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\keys{')), '}');
				$v = array('RawInline' => array('html', '<shortcut>' . convertTexStringToHtml($text) . '</shortcut>'));
			}
			else if (strpos($tex, '\menu{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\menu{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-menu">' . convertTexStringToHtml($text) . '</phrase>'));
			}
			else if (strpos($tex, '\directory{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\directory{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-directory">' . convertTexStringToHtml($text) . '</phrase>'));
			}
			else if (strpos($tex, '\parbox[t]{') === 0)
			{
				preg_match('/\\\\parbox\[t\]{.*}{(.*)}$/', $tex, $matches);
				$v = array('RawInline' => array('html', convertTexStringToHtml($matches[1])));
			}
			else if (trim($tex) === '\newpage') {}
			else if (trim($tex) === '\pagebreak') {}
			else if (trim($tex) === '\nopagebreak') {}
			else if (trim($tex) === '\hfill') {}
			else
				error_log("Warning: ignoring unknown TeX command: $tex\n");

			continue;
		}

		// Convert LaTeX commands into Docbook commands.
		if (is_array($v)
			&& !empty($v['RawBlock'][0])
			&& $v['RawBlock'][0] === 'latex')
		{
			$tex = $v['RawBlock'][1];

			if (strpos($tex, '\begin{lstlisting}') === 0)
			{
				preg_match('/\[language=([a-zA-Z]+).*\]/', $tex, $matches);
				$language = $matches[1];

				$front = strlen('\begin{lstlisting}');
				$back = strlen('\end{lstlisting}');
				$text = substr($tex, $front, -$back);
				$text = substr($text, strpos($text, ']') + 1);

				$v = array('RawBlock' => array('html', '<programlisting language="' . $language . '"><![CDATA[' . $text . ']]></programlisting>'));
			}
			else
				error_log("Warning: ignoring unknown LaTeX command: $tex\n");

			continue;
		}

		if (is_array($v))
			processJSON($v);
	}
}

$book = json_decode(file_get_contents('php://stdin'), true);
processJSON($book);



// Insert the glossary at the end (after the contributor list).
// http://search.cpan.org/~voj/Pandoc-Elements-0.33/lib/Pandoc/Elements.pm
$book[1][] = [ 'Header' => [
	1,
	[ 'glossary', [], [] ],
	[
		[ 'Str' => 'Glossary' ]
	]
]];
$book[1][] = [ 'Para' => [
	[ 'RawInline' => [ 'html', '<abstract>Definitions of words Vuo uses</abstract>' ] ]
]];
uksort($glossary, 'strcasecmp');

// Add a dummy level-2 header so we can use level-3 headers below,
// so each individual term doesn't show up in the table of contents.
$book[1][] = [ 'Header' => [
	2,
	[ '', [], [] ],
	[ [ 'Str' => '' ] ]
]];

// It would be nice to use the more-specific "DefinitionList",
// but I couldn't find a way to get it to emit an element "id"
// so I can create an anchor link to it.
foreach ($glossary as $term => $definition)
{
	$book[1][] = [ 'Header' => [
		3,
		[ makeGlossaryIdentifier($term), [], [] ],
		[ [ 'Str' => $term ] ]
	]];
	$book[1][] = [ 'Para' => [
		[ 'RawInline' => [ 'html', convertTexStringToHtml($definition) ] ]
	]];
}

// Also generate a Mac Help Book Exact Match Searching database.
$exactMatch = '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "https://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>';
foreach ($glossary as $term => $definition)
	$exactMatch .=
		  '<key>' . preg_replace('/[ -]/', '', strtolower($term)) . '</key>'
		. '<string>' . makeGlossaryIdentifier($term) . '</string>';
$exactMatch .= '</dict></plist>';
file_put_contents('ExactMatch.plist', $exactMatch);



print json_encode($book) . "\n";
