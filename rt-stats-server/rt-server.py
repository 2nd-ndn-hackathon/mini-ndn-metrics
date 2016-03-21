#!/usr/bin/env python

import os
import json
from bottle import *

class RtStats:
    def __init__(self, name):
        self.stats = {}
        self.stats['name'] = name
        self.stats['uptime'] = 0
        self.stats['nNameTreeEntries'] = 0
        self.stats['nFibEntries'] = 0
        self.stats['nPitEntries'] = 0
        self.stats['nMeasurementsEntries'] = 0
        self.stats['nCsEntries'] = 0
        self.stats['nInInterests'] = 0
        self.stats['nOutInterests'] = 0
        self.stats['nInDatas'] = 0
        self.stats['nOutDatas'] = 0

    def processLine(self, line):
        line = line.strip().rstrip()
        tokens = line.split('=')
        key = tokens[0]
        value = tokens[1]

        self.stats[key] = value

@get('/rt.html')
def server_static():
    return static_file('rt.html', root='/tmp/stats/')

@get('/stats.json')
def serve_json():
    statsDir = '/tmp/stats/status/rt/'
    statsJson = []

    for filename in os.listdir(statsDir):
        nodeName = filename.strip('.txt')

        stats = RtStats(nodeName)

        with open('{}{}'.format(statsDir, filename), 'r') as inFile:
           for line in inFile:
                stats.processLine(line)

        statsJson.append(stats.stats)

    return json.dumps(statsJson)

run(host='127.0.0.1', port=8081)
