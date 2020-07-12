#!/usr/bin/env php
<?php

function makeIdentifier($s)
{
    return str_replace(' ', '-', preg_replace('/[^a-z0-9 -]/', '', strtolower($s)));
}

date_default_timezone_set('America/New_York');
$vuoVersion = $argv[1];
$book = file_get_contents('php://stdin');

// Replace the Docbook DTD with the local copy, so we don't have to download it every time
$book = str_replace(
	'http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd',
	'/usr/local/opt/docbook/docbook/xml/4.5/docbookx.dtd',
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

// Place each <abstract> tag in a <sectNinfo> tag, where N matches the section it's in.
$book = preg_replace(
	'|(\<sect(\d)[^>]*\>\s*\<title\>.*?\</title\>)\s*\<para\>\s*\<abstract\>(.*?)\<\/abstract\>\s*\<\/para\>|s',
	"$1\n<sect$2info><abstract><para>$3</para></abstract></sect$2info>",
	$book);

// Give each image an individual class, for styling specific images.
$book = preg_replace_callback(
	'|<para>\s*<inlinemediaobject>\s*<imageobject>\s*<imagedata fileref="(image(-generated)?/([A-Za-z0-9-]+).png)" />|m',
	function ($matches) {
		return '<para role="' . makeIdentifier($matches[3]) . '"><inlinemediaobject><imageobject><imagedata fileref="' . $matches[1] . '" />';
	},
	$book);

print $book . "\n";
