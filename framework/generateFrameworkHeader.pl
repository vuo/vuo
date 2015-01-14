#!/usr/bin/perl

# Purpose: Generate the Vuo framework header, given a stub header
# and the names of individual Vuo headers to #include.

use strict;
sub generateVuoIncludeHeaderList;

# placeholder string within the stub header to be replaced
# with the full Vuo header #include list
my $VUO_CXX_HEADER_LIST_PLACEHOLDER = "\@INCLUDE_VUO_CXX_HEADERS\@";
my $VUO_C_HEADER_LIST_PLACEHOLDER = "\@INCLUDE_VUO_C_HEADERS\@";


if ($#ARGV < 0) {
  print "Usage: $0 <stubHeader> [vuoHeader1 ...]\n";
  exit;
}

# Get filename arguments.
my $stubHeader = $ARGV[0];
my ($vuoCxxHeaderList, $vuoCHeaderList) = generateVuoIncludeHeaderList();

# Print the contents of the stub header, inserting the list
# of Vuo headers where appropriate.
my $open = open(STUB_HEADER, $stubHeader);
while (my $line = <STUB_HEADER>) {
    chomp($line);
    $line =~ s/$VUO_CXX_HEADER_LIST_PLACEHOLDER/$vuoCxxHeaderList/g;
    $line =~ s/$VUO_C_HEADER_LIST_PLACEHOLDER/$vuoCHeaderList/g;
    print "$line\n";
  }

close(STUB_HEADER);
exit;


# Generate the #include statement list corresponding to the
# input list of Vuo headers.
sub generateVuoIncludeHeaderList {
  my $cxxHeaderList = "";
  my $cHeaderList = "";

  for (my $i=1; $i<=$#ARGV; $i++) {
    my $curVuoHeader = $ARGV[$i];
    if ($curVuoHeader =~ /\.hh$/) {
      $cxxHeaderList .= "#include \"$curVuoHeader\"\n";
    }
    else {
      $cHeaderList .= "#include \"$curVuoHeader\"\n";
    }
  }
  return ($cxxHeaderList, $cHeaderList);
}
