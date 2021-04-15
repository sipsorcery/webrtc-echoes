#-----------------------------------------------------------------------------
# Filename: collate-results.py
#
# Description: This script collates the results from a set of csv files that
# each contain the result of a single interoperability WebRTC Echo Test. The
# results are combined into a markdown table.
#
# Prerequisites:
# pip install weasyprint
#
# Author(s):
# Aaron Clauson (aaron@sipsorcery.com)
#
# History:
# 19 Feb 2021	Aaron Clauson	Created, Dublin, Ireland.
#
# License: 
# BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
#-----------------------------------------------------------------------------

import os
import glob
import sys
#import pandas as pd
#import dataframe_image as dfi
import weasyprint as wsp
import PIL as pil

from collections import defaultdict
from datetime import datetime

testname = "echo"
if len(sys.argv) > 1: 
    testname = sys.argv[1]

#print("Test name=%s.\n" % testname)
	
# The character width of each cell in the results markdown table.
COL_WIDTH = 12
RESULTS_FILE_PATH = testname + "_test_results.png"

#print("results file path=%s.\n" % RESULTS_FILE_PATH)

def trim(source_filepath, target_filepath=None, background=None):
    if not target_filepath:
        target_filepath = source_filepath
    img = pil.Image.open(source_filepath)
    if background is None:
        background = img.getpixel((0, 0))
    border = pil.Image.new(img.mode, img.size, background)
    diff = pil.ImageChops.difference(img, border)
    bbox = diff.getbbox()
    img = img.crop(bbox) if bbox else img
    img.save(target_filepath)

resultFiles = glob.glob("./*.csv")

results = defaultdict(dict)
serverKeys = []
clientKeys = []

for resFile in resultFiles:
    with open(resFile) as f:
        for line in f:
            cols = line.strip().split(',')
            if not cols[0] in serverKeys:
                serverKeys.append(cols[0])
            if not cols[1] in clientKeys:
                clientKeys.append(cols[1])
            results[cols[0]][cols[1]] = cols[2]

sorted(serverKeys)
sorted(clientKeys)

html = """<html>
 <body>"""

print('## %s Test Results' % testname)
print('Test run at %s\n' % datetime.now())

html += "<h3>" + testname + " Test Results</h3>"
html += "<p>Test run at %s.</p>" % datetime.now()

# Print Table header row.
print(f'| {"Server": <{COL_WIDTH}}| {"Client": <{COL_WIDTH}}', end='')
html += """<table>
 <tr>
   <th>Server</th>
   <th colspan='%d'>Client</th>
 </tr>  
""" % COL_WIDTH

for i in range(0, (len(clientKeys) - 1)):
    print(f'| {" ":<{COL_WIDTH}}', end='')
print('|')

# Print end of header line.
sep = '-' * (COL_WIDTH + 1)
for i in range(len(clientKeys) + 1):
    print(f'|{sep}', end='')
print('|')

# Print Client column headings.
print(f'| {" ":<{COL_WIDTH}}', end='')
html += """<tr>
 <td/>"""
for clientKey in clientKeys:
    print(f'| {clientKey: <{COL_WIDTH}}', end='')
    html += "<td>%s</td>" % clientKey
print('|')
html += "</tr>"

# Print Server rows.
for serverKey in serverKeys:
    print(f'| {serverKey: <{COL_WIDTH}}', end='')
    html += "<tr><td>%s</td>" % serverKey
    for clientKey in clientKeys:
        if serverKey in results.keys() and clientKey in results[serverKey].keys():
            resultChar = '&#9745;' if results[serverKey][clientKey] == '0' else '&#x2612;'
            print(f'| {resultChar:<{COL_WIDTH}}', end='')
            html += "<td>%s</td>" % resultChar
        else:
            print(f'| {" ":<{COL_WIDTH}}', end='')
            html += "<td/>"
    print('|')
    html += "</tr>"

html += """ </body
</html"""

#df = pd.DataFrame(results, columns=clientKeys)
#print(df)
#dfi.export(df, "results.png")

#html = wsp.HTML(string=df.to_html())
#print(html)
html = wsp.HTML(string=html)
html.write_png(RESULTS_FILE_PATH)
trim(RESULTS_FILE_PATH)