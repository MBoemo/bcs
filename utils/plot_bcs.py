#----------------------------------------------------------
# Copyright 2017 University of Oxford
# Written by Michael A. Boemo (mb915@cam.ac.uk)
# This software is licensed under GPL-2.0.  You should have
# received a copy of the license with this software.  If
# not, please Email the author.
#----------------------------------------------------------

import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
import sys
import argparse


#ARGUMENTS--------------------------------------------------------------------------------------------
parser = argparse.ArgumentParser()
parser.add_argument('-a', metavar='action',nargs='+',required=True,help="Action name to track")
parser.add_argument('-p', metavar='process',nargs='+',required=True,help="Process name to track")
parser.add_argument('-i', metavar='parameter',nargs=1,required=True,help="Parameter name to track")
parser.add_argument('-o', metavar='output',nargs=1,required=True,help="Output plot filename (use .png or .pdf extension)")
parser.add_argument('-m', metavar='maxSimulation',nargs=1,help="Maximum number of simulations to plot")
parser.add_argument('filename',help="Output .simulation.bcs file from bcs.")
args = parser.parse_args(sys.argv[1:])
argDict = vars(args)
print argDict


#MAIN-------------------------------------------------------------------------------------------------
f = open(argDict['filename'])
plt.figure()
simCount = 0
x = []
y = []

for line in f:

	if line[0] == '>':

		#do the plotting
		if simCount > 0:
			plt.scatter(x,y)

		x = []
		y = []
		simCount += 1
		if argDict['m'] is not None:
			if simCount >= int(argDict['m'][0]):
				break
		continue

	#parse the line
	splitLine = line.rstrip().split()
	time = float(splitLine[0])
	action = splitLine[1]
	process = splitLine[2]
	if len(splitLine) > 3:	
		parameters = {}
		for i, entry in enumerate(splitLine[3:]):
			if i%2 == 1:
				parameters[splitLine[3:][i-1]] = float(entry)

	if action in argDict['a'] and process in argDict['p'] and argDict['i'][0] in parameters:
		y.append(parameters[argDict['i'][0]])
		x.append(time)
print x
print y
#save figure
if len(x) > 0 and len(y) > 0:
	plt.scatter(x,y)
plt.xlabel('Time')
plt.ylabel(argDict['i'][0])
plt.savefig(argDict['o'][0])

