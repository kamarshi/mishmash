#!/usr/bin/python

import os

import sys

import copy

import random


def findZeros(inlist):
    zeros = []

    for row in xrange(len(inlist)):
        for col in xrange(len(inlist[0])):
            if inlist[row][col] == 0:
                zeros.append((row,col))

    return zeros


def findZeros_better(inlist):
    rzeros = set()

    czeros = set()

    for row in xrange(len(inlist)):
        for col in xrange(len(inlist[0])):
            if inlist[row][col] == 0:
                rzeros.add(row)

                czeros.add(col)

    return rzeros, czeros


def zeroOut_bf(inlist):
    '''
    inlist is a 2D list
    '''
    zeros = []

    zeros = findZeros(inlist)

    for zr, zc in zeros:
        for row in xrange(len(inlist)):
            for col in xrange(len(inlist[0])):
                if zr == row or zc == col:
                    inlist[row][col] = 0


def zeroOut_better(inlist):
    '''
    Two separate layers of processing - rows
    and columns
    '''
    rzeros, czeros = findZeros_better(inlist)

    for zr in rzeros:
        for col in xrange(len(inlist[0])):
            inlist[zr][col] = 0

    for zc in czeros:
        for row in xrange(len(inlist)):
            inlist[row][zc] = 0


def prettyPrint(inlist):
    print '\n'

    for row in xrange(len(inlist)):
        print inlist[row]


def main():
    for indx in xrange(20):
        rows = random.randint(5,20)

        cols = random.randint(5,20)

        rng = max(rows,cols)

        inlist = [[random.randint(0,rng) for r in range(cols)] for x in range(rows)]

        inlist2 = [[0 for r in range(cols)] for x in range(rows)]

        for row in range(rows):
            for col in range(cols):
                inlist2[row][col] = inlist[row][col]

        prettyPrint(inlist)

        zeroOut_bf(inlist)

        prettyPrint(inlist)

        prettyPrint(inlist2)

        zeroOut_better(inlist2)

        prettyPrint(inlist2)

        if inlist != inlist2:
            print 'Bummer!'

            return

        else:
            print 'Success!'


if __name__ == '__main__':
    main()
