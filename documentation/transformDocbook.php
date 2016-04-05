#!/usr/bin/env php
<?php

date_default_timezone_set('America/New_York');
$vuoVersion = $argv[1];
$book = file_get_contents('php://stdin');

// Replace the Docbook DTD with the local copy, so we don't have to download it every time
$book = str_replace(
	'http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd',
	'/usr/local/Cellar/docbook/5.0/docbook/xml/4.5/docbookx.dtd',
	$book);

// Replace Pandoc's bogus <author> tag with something sensible
$book = preg_replace(
	'|\<articleinfo\>.*\<\/articleinfo\>|s',
	'
		<articleinfo>
			<title>Vuo</title>
			<releaseinfo>' . $vuoVersion . '</releaseinfo>
			<pubdate>Revised ' . date('Y.m.d') . '</pubdate>
		</articleinfo>
	',
	$book);

print $book . "\n";
