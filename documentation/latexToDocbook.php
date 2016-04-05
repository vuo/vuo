#!/usr/bin/env php
<?php

function convertTexStringToHtml($s)
{
	$texToHtml = array(
		'\doublequote' => '"',
		'\textbackslash' => '\\',
		'\textgreater' => '&gt;',
		'\textasciitilde' => '~',
		'\textunderscore' => '_',
		'\$' => '$',
		'\ ' => ' ',
		'``' => '&ldquo;',
		"''" => '&rdquo;',
		'<' => '&lt;',
		'>' => '&gt;',
		' --- ' => ' &mdash; '
	);
	foreach ($texToHtml as $tex => $html)
	{
		$s = str_replace($tex . '{}', $html, $s);
		$s = str_replace($tex, $html, $s);
	}
	$s = preg_replace('/\\\\vuoPort{([^}]+)}/', '<phrase role="vuo-port">$1</phrase>', $s);
	$s = preg_replace('/\\\\vuoNode{([^}]+)}/', '<phrase role="vuo-node">$1</phrase>', $s);
	$s = preg_replace('/\\\\term{([^}]+)}/', '<phrase role="vuo-term">$1</phrase>', $s);
	$s = preg_replace('/\\\\code{([^}]+)}/', '<literal>$1</literal>', $s);
	$s = preg_replace('/\\\\keys{([^}]+)}/', '<shortcut>$1</shortcut>', $s);

	// Special case for Keyboard Shortcuts table
	$s = preg_replace('/\\\\hbox{\\\\strut ([^}]+)}\\\\hbox{\\\\strut ([^}]+)/', "<literallayout>$1\n$2</literallayout>", $s);

	return $s;
}

function processJSON(&$book)
{
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
			else if (preg_match('/\\includegraphics\[scale=([0-9.]+)\]{([^}]+)\.pdf}/', $tex, $matches))
				$v = array('Image' => array(array(), array($matches[2] . '.png', '')));
			else if (strpos($tex, '\code{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\code{')), '}');
				$v = array('Code' => array(array("",array(),array()), convertTexStringToHtml($text)));
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
			else if (strpos($tex, '\noteQCUsers{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\noteQCUsers{')), '}');
				$v = array('RawInline' => array('html', '<para role="vuo-note-qc">' . convertTexStringToHtml($text) . '</para>'));
			}
			else if (strpos($tex, '\noteTextProgrammers{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\noteTextProgrammers{')), '}');
				$v = array('RawInline' => array('html', '<para role="vuo-note-text">' . convertTexStringToHtml($text) . '</para>'));
			}
			else if (strpos($tex, '\term{') === 0)
			{
				$text = rtrim(substr($tex, strlen('\term{')), '}');
				$v = array('RawInline' => array('html', '<phrase role="vuo-term">' . convertTexStringToHtml($text) . '</phrase>'));
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
			else if (trim($tex) === '\newpage') {}
			else if (trim($tex) === '\pagebreak') {}
			else if (trim($tex) === '\nopagebreak') {}
			else if (trim($tex) === '\hfill') {}
			else if (strpos($tex, '\vtop{') === 0)	// Special case for Keyboard Shortcuts table
			{
				$text = rtrim(substr($tex, strlen('\vtop{')), '}');
				$v = array('RawInline' => array('html', convertTexStringToHtml($text)));
			}
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
				preg_match('/\[language=([a-z]+).*\]/', $tex, $matches);
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
print json_encode($book) . "\n";
