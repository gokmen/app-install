#!/usr/bin/python
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

# Copyright (C) 2009
#    Richard Hughes <richard@hughsie.com>
#
# What this script does:
#
# 1. gets a list of all the packages in a repo that ship a desktop file
# 2. downloads all the files and extracts them to $dist/root
# 3. process each desktop file using pk-ap-install-generate and pipe the output
#    to a file. This tool also copies the correct icons.

import os
import sys
import getopt
import yum
import shutil
import tarfile
import subprocess

def usage():
    print "yum-app-install-generate.py --repo=rawhide --dist=./dist"
    print "  All packages will be downloaded and unpacked, and then the SQL"
    print "  will be automatically generated."

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hrd:v", ["help", "repo=", "dist="])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # parse command line arguments
    reponame = None
    dist = None
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-r", "--repo"):
            reponame = a
        elif o in ("-d", "--dist"):
            dist = a
        else:
            assert False, "unhandled option"

    # check all exists okay
    if not reponame or not dist:
        usage()
        sys.exit()

    # setup yum
    yb = yum.YumBase()
    yb.doConfigSetup(errorlevel=-1, debuglevel=-1)
    yb.conf.cache = 0

    # open sql file
    db = dist + "/%s.sqldata" % reponame
    print "opening", db
    outp = open(db,"w")

    # create a cache directory
    if not os.path.exists(dist + '/cache'):
        os.makedirs(dist + '/cache')

    # create a icons directory
    if not os.path.exists(dist + '/icons'):
        os.makedirs(dist + '/icons')

    # find all packages
    pkgs = yb.pkgSack
    licenses = []
    for pkg in pkgs:
        if pkg.repoid != reponame:
            continue

        repo = yb.repos.getRepo(pkg.repoid)
        print "package name:", pkg.name

        # find out if any of the files ship a desktop file
        desktop_files = []
        for instfile in pkg.returnFileEntries():
            if instfile.startswith('/usr/share/applications/') and instfile.endswith('.desktop'):
                print "filename:", instfile[24:]
                desktop_files.append(instfile[24:])

        # don't download packages without desktop files
        if len(desktop_files) == 0:
            continue

        # add license string
        if pkg.license not in licenses:
            licenses.append(pkg.license)

        # get base name without the slash
        relativepath = pkg.returnSimple('relativepath')
        pos = relativepath.rfind('/')
        if pos != -1:
            relativepath = relativepath[pos+1:]

        # is in cache?
        path = dist + '/cache/' + relativepath
        if os.path.exists(path) and os.path.getsize(path) == int(pkg.returnSimple('packagesize')):
            print "already downloaded!"
        else:
            pkg.localpath = path
            print 'downloading', path
            repo.getPackage(pkg)

        # create a root directory
        if not os.path.exists(dist + '/root'):
            os.makedirs(dist + '/root')

        # extract
        directory = dist + '/root'
        path = '../cache/' + relativepath
        cmd = "rpm2cpio %s | cpio --extract --make-directories --quiet" % path
        print 'extracting', path
        p = subprocess.Popen(cmd, cwd=directory, shell=True, stdout=subprocess.PIPE)
        p.wait()
        if p.returncode:
            print 'cannot extract package', relativepath
            for line in p.stdout:
                print line
            continue

        # find desktop files
        for instfile in desktop_files:
            print 'generating sql for', instfile
            cwd = os.getcwd()
            cmd = "app-install-generate --outputdir=%s --repo=%s --root=%s --desktopfile=%s --package=%s" % (dist, pkg.repoid, directory, instfile, pkg.name)
            p = subprocess.Popen(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE)
            p.wait()
            if p.returncode:
                print 'failed to generate sql for', instfile, 'using', cmd, ':'
                for line in p.stdout:
                    print line
                continue
            for sql in p.stdout:
                outp.write(sql)

        # do this per package else it takes ages at the end
        print 'removing temporary files'
        shutil.rmtree(dist + '/root')

    # get license string
    license_string = ''
    for license in licenses:
        if license.find(' ') != -1:
            license = '(' + license + ')'
        license_string += "%s and " % license
    print 'license = ', license_string

    print 'finished file write'
    outp.close()

    # create tar archive
    print 'creating archive'
    cwd = os.getcwd()
    os.chdir(dist)
    iconfile = "%s-icons.tar.gz" % reponame
    tar = tarfile.open(iconfile, "w:gz")
    tar.add('icons')
    tar.close()
    os.chdir(cwd)

    print 'removing any remaining temporary files'
    shutil.rmtree(dist + '/icons')

if __name__ == "__main__":
    main()

