#!/usr/bin/python

import os
import sys
import re

class gps_parser(object):

    def __init__(self, infname, outfname):
        self.talker = 'GP'
        self.sid = ['GGA', 'GLL', 'RMC']
        self.sfile = open(infname, 'r')
        self.outfile = open(outfname, 'w')

    def checksum(self, line):
        """
        Checksum field exists, compute it
        """
        csl = line.split('*')
        csl[0] = csl[0][1:]
        cs = 0
        for item in csl[0]:
            val = ord(item)
            cs = cs ^ val

        if cs != int(csl[1], base=16):
            print 'cs mismatch: ', cs, ':', int(csl[1], base=16)
            return False
        else:
            return True

    def getline(self):
        """
        Get a line from file, look for basic errors
        """
        line = self.sfile.readline().rstrip()
        if len(line) < 1:
            return ('', False)
        if len(line) > 81:
            print 'Sentence is longer than allowed'
            return (line, False)
        match = re.search('\\*', line)
        good = False
        if match is not None:
            good = self.checksum(line)
            if good:
                line = line[1:match.start()]
        else:
            good = True
            line = line[1:]
        if not good:
            print 'Checksum failed'
            return (line, False)
        return (line, True)

    def lat_to_decimal(self, coord, direction):
        """
        Convert from xx(deg)yy.yy(min) to decimal degrees for lat
        Add sign if direction is 'S'
        Return signed decimal number in string format
        """
        deg = float(coord[:2])
        mins = float(coord[2:])
        deg = deg + mins / 60.0
        sdeg = str(deg)
        if direction == 'S':
            sdeg = '-' + sdeg
        return sdeg

    def lon_to_decimal(self, coord, direction):
        """
        Convert from xxx(deg)yy.yy(min) to decimal degrees for lon
        Add sign if direction is 'W'
        Return signed decimal number in string format
        """
        deg = float(coord[:3])
        mins = float(coord[3:])
        deg = deg + mins / 60.0
        sdeg = str(deg)
        if direction == 'W':
            sdeg = '-' + sdeg
        return sdeg

    def parse_sentence(self, sid, line):
        """
        Parses line with sentence ID sid.  Returns:
        (utc - Universal time code
         lat - latitude in decimal format
         lon - longitude in decimal format
         isvalid - True if receiver status is 'A'
        """
        sline = line.split(',')
        isvalid = True
        if sid == 'RMC':
            utc = sline[1][:2] + ':' + sline[1][2:4] + ':' + sline[1][4:6]
            lat = self.lat_to_decimal(sline[3], sline[4])
            lon = self.lon_to_decimal(sline[5], sline[6])
            if sline[2] != 'A':
                isvalid = False
        elif sid == 'GLL':
            utc = sline[5][:2] + ':' + sline[5][2:4] + ':' + sline[5][4:6]
            lat = self.lat_to_decimal(sline[1], sline[2])
            lon = self.lon_to_decimal(sline[3], sline[4])
            if sline[6] != 'A':
                isvalid = False
        else:
            utc = sline[1][:2] + ':' + sline[1][2:4] + ':' + sline[1][4:6]
            lat = self.lat_to_decimal(sline[2], sline[3])
            lon = self.lon_to_decimal(sline[4], sline[5])
        return (utc,
         lat,
         lon,
         isvalid)

    def parse_data(self):
        """
        Loop over lines, look for target sentences and
        print data.  Bug out on error
        """
        while True:
            line, status = self.getline()
            if not status:
                if len(line) == 0:
                    print 'end of file'
                    break
                else:
                    print 'Error parsing line.  Skipping ...: ', line
                    continue
            sentence = line.split(',')
            sid = None
            for item in self.sid:
                if self.talker + item in sentence:
                    sid = item

            if sid is not None:
                utc, lat, lon, isvalid = self.parse_sentence(sid, line)
                if isvalid:
                    self.outfile.write('{0:8}, {1:12}, {2:12}\n'.format(utc, lat, lon))

        self.outfile.close()


def main():
    gpsp = gps_parser('sample3.txt', 'sample3.ref')
    gpsp.parse_data()


if __name__ == '__main__':
    main()
