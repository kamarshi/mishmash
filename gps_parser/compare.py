#!/usr/bin/python

import os

import sys

def  compare(infname, outfname):
    print 'Comparing files - ', infname, ' and ', outfname

    infp = open(infname)

    outfp = open(outfname)

    inall = infp.readlines()

    outall = outfp.readlines()

    if len(inall) != len(outall):
        print 'Files have differing line counts'

        return

    for indx, line in enumerate(inall):
        inl = line.strip().split(',')

        outl = outall[indx].strip().split(',')

        # print 'test: ', inl, '  ', 'ref: ', outl

        if inl[0] != outl[0]:
            print 'UTC mismatch: ', inl[0], '  ', outl[0]

            return

        if abs(float(inl[1]) - float(outl[1])) > 0.001:
            print 'Lat values are different: ', inl[1], '   ', outl[1]

            return

        if abs(float(inl[2]) - float(outl[2])) > 0.001:
            print 'Lon values are different: ', inl[2], '   ', outl[2]

            return

    print 'Success!'


def main():
    compare('sample3.out', 'sample3.ref')


if __name__ == '__main__':
    main()
