<?php
// Parses the Material color palette SCSS file and converts it into JSON.

$url = 'https://github.com/material-components/material-components-web/raw/master/packages/mdc-theme/_color-palette.scss';
$lines = explode("\n", file_get_contents($url));
$colors = [];
foreach ($lines as $line) {
  if (!preg_match('/\$material-color-(.*)-(.*): (#.*);$/', $line, $matches))//empty($line) || $line[0] !== '$')
    continue;

  $colors[$matches[1]][$matches[2]] = $matches[3];
}

echo "// Generated from $url\n\n";
echo "static const char *materialColors = VUO_STRINGIFY(";
echo json_encode($colors);
echo ");\n";
