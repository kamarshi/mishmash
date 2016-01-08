#!/usr/bin/python

# Started at 4 pm 1/6/16

import os

import sys

import re

import random

from gps_parser import gps_parser


class gps_generator(gps_parser):
    '''
    Parses data from input file.  If sentence is valid,
    uses it to randomly create 0, 1 or 2 new sentences
    with the same sentence ID, but different UTC and
    lat/lon values.  Copies all input data to output
    file, plus generated sentences
    '''
    def __init__(self, infname, outfname):
        super(self.__class__, self).__init__(infname, outfname)


    def create_utc(self):
        hh = random.randrange(0, 24)

        mm = random.randrange(0, 60)

        ss = random.randrange(0, 60)

        fss = random.randrange(0, 100)

        new_utc = str(hh).zfill(2) + str(mm).zfill(2) + str(ss).zfill(2) + '.' + str(fss).zfill(2)

        return new_utc


    def create_lat(self):
        deg = random.randrange(0, 90)

        mins = random.randrange(0, 60)

        fmins = random.randrange(0, 100)

        new_lat = str(deg).zfill(2) + str(mins).zfill(2) + '.' + str(fmins).zfill(2)

        return new_lat


    def create_lon(self):
        deg = random.randrange(0, 180)

        mins = random.randrange(0, 60)

        fmins = random.randrange(0, 100)

        new_lat = str(deg).zfill(2) + str(mins).zfill(2) + '.' + str(fmins).zfill(2)

        return new_lat


    def regenerate(self, line):
        '''
        Regenerate line - 
        1. Add '$' in front
        2. Compute checksum and add to end
        '''
        cs = 0

        for item in line:
            val = ord(item)

            cs = cs ^ val

        css = hex(cs)[2:].zfill(2).upper()

        outline = '$' + line + '*' + css + '\r\n'

        return outline


    def regenerate_new(self, sid, line):
        '''
        Regenerate line - 
        1. Synthesize utc, lat, lon
        2. patch in utc, lat, lon
        3. Call up regenerate
        '''
        new_utc = self.create_utc()

        new_lat = self.create_lat()

        new_lon = self.create_lon()

        sline = line.split(',')

        # Not bothering to change direction
        if sid == 'RMC':
            sline[1] = new_utc

            sline[3] = new_lat

            sline[5] = new_lon

        elif sid == 'GLL':
            sline[5] = new_utc

            sline[1] = new_lat

            sline[3] = new_lon

        else:
            # sid is GGA
            sline[1] = new_utc

            sline[2] = new_lat

            sline[4] = new_lon

        newline = ','.join(sline)

        print 'New generated line: ', newline, ':', sid

        return self.regenerate(newline)


    def parse_data(self):
        '''
        Loop over lines, look for target sentences and
        print data.  Bug out on error
        '''
        while True:
            line, status = self.getline()

            if not status:
                if len(line) == 0:
                    print 'end of file'

                    break

                else:
                    print 'Error parsing line.  Outputting it to generated file anyway: ', line

                    outline = self.regenerate(line)

                    self.outfile.write(outline)

                    continue

            # First copy this line to outfile
            outline = self.regenerate(line)

            self.outfile.write(outline)

            sentence = line.split(',')

            sid = None

            for item in self.sid:
                if self.talker + item in sentence:
                    sid = item

            if sid is not None:
                utc, lat, lon, isvalid = self.parse_sentence(sid, line)

                if isvalid:
                    randval = random.randint(0,3)

                    for indx in range(randval):
                        outline = self.regenerate_new(sid, line)

                        self.outfile.write(outline)

        self.outfile.close()


def main():
    gpsgen = gps_generator('sample2.txt', 'sample3.txt')

    gpsgen.parse_data()


if __name__ == '__main__':
    main()

