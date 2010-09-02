#!/usr/bin/perl
# Licensed under the GNU General Public License Version 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Copyright (C) 2010 Daniel Nicoletti <dantti85-pk@yahoo.com.br>
use warnings;
use strict;
use Getopt::Long;

my $prefix;
my $database = '/usr/share/app-install/desktop.db';
my $help;
my $result = GetOptions(
    "prefix=s" => \$prefix,
    "database=s" => \$database,
    "help" => \$help
    );

if ($help) {
    print "prefix the were app-install-generate is\n";
    print "USAGE cmd --prefix=PATH --database=PATH\n";
    exit;
}

if (!$prefix) {
    print "Missing prefix, try --help\n";
    exit;
}

# This directory contains all the *.desktop files
my @desktops = </usr/share/app-install/desktop/*.desktop>;

foreach my $desktop (@desktops) {
    open(DESKTOP, $desktop) or die("Could not open desktop file.");
    foreach my $line (<DESKTOP>) {
        chomp($line);
        # Get the package that it belongs to
        if ($line =~ m/^X-AppInstall-Package=(.*)/) {
            print "${prefix}app-install-generate --repo=main --icondir=/tmp --desktopfile=$desktop --package=$1 --database=$database\n";
            `${prefix}app-install-generate --repo=main --icondir=/tmp --desktopfile=$desktop --package=$1 --database=$database`;
            last;
        }
    }
}
