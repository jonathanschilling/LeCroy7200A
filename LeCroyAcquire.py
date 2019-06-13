#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Oct 28 20:57:10 2018

@author: jonathan
"""

import gpib
import numpy as np

# GPIB interface 0, address 15
con = gpib.dev(0,15)

status = gpib.write(con, "COMM_FORMAT OFF")
#status = gpib.write(con, "COMM_FORMAT OFF,WORD,BIN")
status = gpib.write(con, "COMM_HEADER OFF")

status = gpib.write(con, "*IDN?")
deviceID = gpib.read(con, 1000).decode()
print("found device: " + deviceID)


# get template
print("fetching template")
status = gpib.write(con, "TEMPLATE?")
template = ""

chunk_size = 1024
keepFetching = True
while keepFetching:
    temp = gpib.read(con, chunk_size).decode()
    template += temp
    print("read " + np.str(len(template)) + " to " + np.str(len(template)+len(temp)))
    if len(temp) < chunk_size:
        keepFetching = False

with open("template.txt", "w") as f:
    f.write(template)


# fetch trace 1
print("fetching T1...")
status = gpib.write(con, "T1: WF?")
trace1 = b''

chunk_size = 1024
keepFetching = True
while keepFetching:
    temp = gpib.read(con, chunk_size)
    trace1 += temp
    print("read " + np.str(len(trace1)) + " to " + np.str(len(trace1)+len(temp)))
    if len(temp) < chunk_size:
        keepFetching = False

with open("trace1.000", "wb") as f:
    f.write(trace1)

# fetch trace 2
print("fetching T2...")
status = gpib.write(con, "T2: WF?")
trace2 = b''

chunk_size = 1024
keepFetching = True
while keepFetching:
    temp = gpib.read(con, chunk_size)
    trace2 += temp
    print("read " + np.str(len(trace2)) + " to " + np.str(len(trace2)+len(temp)))
    if len(temp) < chunk_size:
        keepFetching = False

with open("trace2.000", "wb") as f:
    f.write(trace2)

# fetch trace 3
print("fetching T3...")
status = gpib.write(con, "T3: WF?")
trace3 = b''

chunk_size = 1024
keepFetching = True
while keepFetching:
    temp = gpib.read(con, chunk_size)
    trace3 += temp
    print("read " + np.str(len(trace3)) + " to " + np.str(len(trace3)+len(temp)))
    if len(temp) < chunk_size:
        keepFetching = False

with open("trace3.000", "wb") as f:
    f.write(trace3)

# fetch trace 2
print("fetching T4...")
status = gpib.write(con, "T4: WF?")
trace4 = b''

chunk_size = 1024
keepFetching = True
while keepFetching:
    temp = gpib.read(con, chunk_size)
    trace4 += temp
    print("read " + np.str(len(trace4)) + " to " + np.str(len(trace4)+len(temp)))
    if len(temp) < chunk_size:
        keepFetching = False

with open("trace4.000", "wb") as f:
    f.write(trace4)