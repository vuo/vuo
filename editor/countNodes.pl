#!/usr/bin/perl 

# Purpose: Counts the number of instances of each node class within the current set of example compositions.

use strict;

# Process command-line arguments.
if (($#ARGV < 0) || (($#ARGV == 1) && ($ARGV[1] ne "-c")) || ($#ARGV > 1)) {
  print "Usage: $0 <nodeDir> [-c]\n";
  print "   <nodeDir>\tthe \"node\" directory within the vuo project, e.g, \"vuo/trunk/node\"\n";
  print "   -c\t\tprint results in ready-to-paste C++-map format\n";
  exit;
}

my $nodeDir = $ARGV[0];
my $mapFormattedOutput = ($ARGV[1] eq "-c");

# Validate the input node directory.
if (! -d $nodeDir) {
  print "Error: Input invalid node directory: '$nodeDir'.\n";
  exit;
}

my @nodeSetDirs;
opendir(DIR, $nodeDir) or die $!;

# Derive list of node sets.
while (my $subdir = readdir(DIR)) {
  if ((-d "$nodeDir/$subdir") &&
      ($subdir !~ m/^\./) &&
      ($subdir !~ m/^descriptions/) &&
      ($subdir !~ m/^examples/)) {
    push(@nodeSetDirs, "$nodeDir/$subdir");
  }
}

# Derive list of node sets with pro content.
foreach my $nodeSetDir (@nodeSetDirs) {
  opendir(DIR, $nodeSetDir) or die $!;
  while (my $subdir = readdir(DIR)) {
      if ((-d "$nodeSetDir/$subdir") &&
	  ($subdir eq "pro"))
	{
	  push(@nodeSetDirs, "$nodeSetDir/$subdir");
	}
    }
}

# Derive list of node classes (pro and non-pro) and example composition directories.
my @nodeClassNames;
my @exampleCompositionDirs;
foreach my $nodeSetDir (@nodeSetDirs) {
  opendir(DIR, $nodeSetDir) or die $!;
  while (my $file = readdir(DIR)) {
    if ((-f "$nodeSetDir/$file") &&
	# Assumption: node class implementations are contained in vuo.*.{c,cc} files.
	($file =~ m/^vuo\..*\.cc?$/)) {

      (my $nodeClassName = $file) =~ s/\.cc?$//;
      push(@nodeClassNames, $nodeClassName);
    }
    elsif ((-d "$nodeSetDir/$file") &&
	   ($file eq "examples")) {
      push(@exampleCompositionDirs, "$nodeSetDir/$file");
    }
  }
}

# Derive list of example compositions.
my @exampleCompositions;
foreach my $exampleCompositionDir (@exampleCompositionDirs) {
  opendir(DIR, $exampleCompositionDir) or die $!;
  while (my $file = readdir(DIR)) {
    if ((-f "$exampleCompositionDir/$file") &&
	($file =~ m/\.vuo$/)) {
      push(@exampleCompositions, "$exampleCompositionDir/$file");
    }
  }
}
 
# Count number of occurrences of each node class within example compositions.
my %nodeClassFrequency;

foreach my $nodeClassName (@nodeClassNames) {
  $nodeClassFrequency{$nodeClassName} = 0;
}

foreach my $exampleComposition (@exampleCompositions) {
    open(FILE, $exampleComposition) or die $!;
    my @contents = <FILE>;
    close(FILE);

    foreach my $nodeClassName (@nodeClassNames) {
      my @matchingContents = grep(/type=\"$nodeClassName(\.Vuo.*)?\"/, @contents);
      foreach my $match (@matchingContents) {
	$nodeClassFrequency{$nodeClassName}++;
    }
  }
}

# Print node classes and frequencies.
if ($mapFormattedOutput) {
  my $datestring = localtime();
  print "// This file was automatically generated on $datestring by countNodes.pl.\n";
}
else {
  print "# Node class\t\t# Frequency\n";
}
foreach my $nodeClass (reverse sort {$nodeClassFrequency{$a} <=> $nodeClassFrequency{$b} } keys %nodeClassFrequency) {
  if ($mapFormattedOutput) {
    print "nodeClassFrequency[\"$nodeClass\"] = $nodeClassFrequency{$nodeClass};\n";
  }
  else {
    print "$nodeClass\t$nodeClassFrequency{\"$nodeClass\"}\n";
  }
}

exit;
