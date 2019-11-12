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

#--------------------------------------------------------------------------------------------------------------------------------------
class arguments:
	pass


#--------------------------------------------------------------------------------------------------------------------------------------
def splashHelp():
	s = """plot_bcs.py: Plotting utility for the Beacon Calculus Simulator (bcs).
To run plot_bcs.py, do:
  python plot_bcs.py [arguments] [bcs output]
Example:
  python plot_bcs.py -m /path/to/thymidine_model -e /path/to/nanopolish_eventalignment -o outputPlot.pdf bcsOutput.simulation.bcs
Required arguments are:
  -m,--ont_model            path to thymidine-only ONT model,
  -e,--alignment            path to Nanopolish eventalign output file,
  -o,--output               output prefix,
  -n,--maxReads             maximum number of reads to import from eventalign."""

	print s
	exit(0)


#--------------------------------------------------------------------------------------------------------------------------------------
def parseArguments(args):

	a = arguments()
	a.clipToMax = False
	a.maxReads = 1

	for i, argument in enumerate(args):
			
		if argument == '-m' or argument == '--ont_model':
			a.ont_model = str(args[i+1])

		elif argument == '-e' or argument == '--alignment':
			a.eventalign = str(args[i+1])

		elif argument == '-o' or argument == '--output':
			a.outFile = str(args[i+1])

		elif argument == '-n' or argument == '--maxReads':
			a.maxReads = int(args[i+1])
			a.clipToMax = True 

		elif argument == '-h' or argument == '--help':
			splashHelp()

	#check that required arguments are met
	if not hasattr( a, 'ont_model') or not hasattr( a, 'eventalign') or not hasattr( a, 'outFile') or not hasattr( a, 'maxReads'):
		splashHelp() 

	return a


#--------------------------------------------------------------------------------------------------------------------------------------
def displayProgress(current, total):

	barWidth = 70
	progress = float(current)/float(total)

	if progress <= 1.0:
		sys.stdout.write('[')
		pos = int(barWidth*progress)
		for i in range(barWidth):
			if i < pos:
				sys.stdout.write('=')
			elif i == pos:
				sys.stdout.write('>')
			else:
				sys.stdout.write(' ')
		sys.stdout.write('] '+str(int(progress*100))+' %\r')
		sys.stdout.flush()




