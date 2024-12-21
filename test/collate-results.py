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
# 21 Aug 2021	Aaron Clauson	Investigating breaking change to weasyprint that removed the write to png function, see
#                               https://www.courtbouillon.org/blog/00004-weasyprint-without-cairo-what-s-different.
# 12 Oct 2023   Aaron Clauson   Swtiched from csv file processing to stdin. Removed weasyprint html to png stage.
#
# License: 
# BSD 3-Clause "New" or "Revised" License, see included LICENSE.md file.
#-----------------------------------------------------------------------------

import json
import sys

from collections import defaultdict
from datetime import datetime

testname = "PeerConnection"

# Read JSON input from stdin (for GitHub Actions)
input_data = sys.stdin.read()

# Parse the input JSON
try:
    results = json.loads(input_data)
except json.JSONDecodeError as e:
    print(f"Failed to parse JSON: {e}")
    sys.exit(1)

# Initialize data structures
serverKeys = []
clientKeys = []

# Prepare the results structure
results_dict = defaultdict(dict)

# Populate the results_dict with data from the parsed JSON
for key, value in results.items():
    server, client = key.split("_")[1:]  # Assuming keys are formatted as "result_<server>_<client>"
    results_dict[server][client] = value
    if server not in serverKeys:
        serverKeys.append(server)
    if client not in clientKeys:
        clientKeys.append(client)

# Sort the server and client keys
serverKeys.sort()
clientKeys.sort()

# Print Markdown table header

print('Test run at %s\n' % datetime.now())

# Print Table header row.
print(f'| {"Server": <12} | {" | ".join(clientKeys)} |')
print('|--------|' + '|'.join(['--------'] * len(clientKeys)) + '|')

# Print Server rows.
for serverKey in serverKeys:
    print(f'| {serverKey: <12} ', end='')
    for clientKey in clientKeys:
        if clientKey in results_dict[serverKey]:
            resultChar = '✔' if results_dict[serverKey][clientKey] == '0' else '✘'
            print(f'| {resultChar: <7}', end='')
        else:
            print(f'| {"✘":<7}', end='')
    print('|')
