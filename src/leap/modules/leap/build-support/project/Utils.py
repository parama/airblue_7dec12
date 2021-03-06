############################################################################
############################################################################
##
## Utility functions used in this module and in submodules
##
############################################################################
############################################################################

import os
import re
import sys
import string
import subprocess

##
## clean_split --
##     Split a string into a list using specified separator (default ':'),
##     dropping empty entries.
##
def clean_split(list, sep=':'):
    return [x for x in list.split(sep) if x != '' ]

##
## rebase_directory --
##     Rebase directory (d) that is a reference relative to the root build
##     directory, returning a result relative to cwd.  cwd must also be
##     relative to the root build directory.
##
def rebase_directory(d, cwd):
    d = clean_split(d, sep='/')
    cwd = clean_split(cwd, sep='/')

    for x in cwd:
        if (len(d) == 0 or d[0] != x):
            d.insert(0, '..')
        else:
            d.pop(0)

    if (len(d) == 0): d = [ '.' ]
    return '/'.join(d)

##
## transform_string_list --
##     Interpret incoming string (str) as a list of substrings separated by (sep).
##     Add (prefix) and (suffix) to each substring and return a modified string.
##
def transform_string_list(str, sep, prefix, suffix):
    if (sep == None):
        sep = ' '
    t = [ prefix + a + suffix for a in clean_split(str, sep) ]
    return string.join(t, sep)
    
##
## one_line_cmd --
##     Issue a shell command and return the first line of output
##
def one_line_cmd(cmd):
    p = os.popen(cmd)
    r = p.read().rstrip()
    p.close()
    return r

def execute(cmd):
    p = subprocess.Popen(cmd, shell=True)
    sts = os.waitpid(p.pid, 0)[1]
    return sts

##
## awb_resolver --
##     Ask awb-resolver for some info.  Return the first line.
##
def awb_resolver(arg):
    return one_line_cmd("awb-resolver " + arg)

##
## get_bluespec_verilog --
##     Return a list of Verilog files from the Bluespec compiler release.
##
def get_bluespec_verilog(env):
    resultArray = []
    bluespecdir = env['ENV']['BLUESPECDIR']
    
    fileProc = subprocess.Popen(["ls", "-1", bluespecdir + '/Verilog/'], stdout = subprocess.PIPE)
    fileList = fileProc.stdout.read()
    #print fileList
    fileArray = clean_split(fileList, sep = '\n')
    for file in fileArray:
        if ((file[-2:] == '.v') and
            (file != 'main.v') and
            (file != 'ConstrainedRandom.v')):
            resultArray.append(bluespecdir + '/Verilog/' + file)
    return resultArray


##
## get_bluespec_xcf --
##     Return a list of XCF files associated with Bluespec provided libraries.
##
def get_bluespec_xcf(env):
    bluespecdir = env['ENV']['BLUESPECDIR']

    # Bluespec only provides board-specific XCF files, but for now they are
    # all the same.  Find one.
    xcf = bluespecdir + '/board_support/xilinx/XUPV5/default.xcf.template'
    if os.path.exists(xcf):
        return [ xcf ];
    else:
        return [];


def getBluespecVersion():
    # What is the Bluespec compiler version?
    bsc_version = 0

    bsc_ostream = os.popen('bsc -verbose')
    ver_regexp = re.compile('^Bluespec Compiler, version.*\(build ([0-9]+),')
    for ln in bsc_ostream.readlines():
        m = ver_regexp.match(ln)
        if (m):
            bsc_version = int(m.group(1))
    bsc_ostream.close()

    if bsc_version == 0:
        print "Failed to get Bluespec compiler version"
        sys.exit(1)

    return bsc_version

# useful for reconstructing synthesis boundary dependencies
# returns a list of elements with exactly the argument filepath 
def checkFilePath(prefix, path):
   (filepath,filname) = os.path.split(path)
   return prefix == path

