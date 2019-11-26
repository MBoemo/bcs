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


#MAIN-------------------------------------------------------------------------------------------------
f = open(argDict['filename'])
plt.figure()
simCount = 0

process2x = {}
process2y = {}
x = []
y = []

for line in f:

	if line[0] == '>':

		#do the plotting
		if simCount > 0:
			for key in process2x:
				plt.scatter(process2x[key],process2y[key],alpha=0.3,label=key+' (sim'+str(simCount)+')')

		process2x.clear()
		process2y.clear()
		if argDict['m'] is not None:
			if simCount >= int(argDict['m'][0]):
				break
		simCount += 1
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
		if process in process2y:
			process2y[process].append(parameters[argDict['i'][0]])
		else:
			process2y[process] = [parameters[argDict['i'][0]]]
		if process in process2x:
			process2x[process].append(time)
		else:
			process2x[process] = [time]

#save figure
if any(process2x) and any(process2y):
	for key in process2x:
		plt.scatter(process2x[key],process2y[key],alpha=0.3,label=key+' (sim'+str(simCount)+')')
plt.xlabel('Time')
plt.ylabel(argDict['i'][0])
plt.legend(framealpha=0.3)
plt.savefig(argDict['o'][0])
