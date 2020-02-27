#!/usr/bin/env php
<?php
$language = $argv[1];
$qtStringsFile = $argv[2];
$vuoStringsFile = $argv[3];
$markdown = file_get_contents('php://stdin');

if ($language != 'en')
{
	global $translations;
	$translations = [];
	function parseStringsFile($filename)
	{
		$ts = simplexml_load_file($filename);
		global $translations;
		foreach ($ts->xpath('//message') as $message)
			$translations[(string)$message->source[0]] = (string)$message->translation[0];
	}
	// Parse Vuo second so it can override Qt's defaults if needed.
	parseStringsFile($qtStringsFile);
	parseStringsFile($vuoStringsFile);

	// For each `\menu{…}` tag in the manual…
	$markdown = preg_replace_callback(
		'@\\\\menu{([^}]+)}@',
		function ($matches) {
			// Split it into its individual menu items.
			$menuLevels = explode(' > ', $matches[1]);

			// Translate each menu item name individually.
			global $translations;
			foreach($menuLevels as &$l)
			{
				$l = str_replace('\&', '&', $l);

				if (isset($translations[$l]))
					$l = $translations[$l];
				// else if (!strstr(strtolower($l), 'vuo'))
				// 	error_log("warning: Untranslated string '$l'");

				$l = str_replace('&', '\&', $l);
			}

			return '\\menu{' . join(' > ', $menuLevels) . '}';
		},
		$markdown);
}

print $markdown . "\n";
