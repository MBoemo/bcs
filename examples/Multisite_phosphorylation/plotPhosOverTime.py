import sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
import numpy as np

plt.figure()
maxLines = 30
plotted = 0

totalPhosphorylation = 0
y = []
x = []
ends = []
f = open(sys.argv[1],'r')
for line in f:

	if line[0] == '>':
		if len(x) > 0 and plotted <= maxLines:
			plt.plot(x,y,alpha=0.2)
			plotted += 1
			totalPhosphorylation = 0
			ends.append(y[-1:])
			y = []
			x = []
			
		 
	else:
		splitLine = line.rstrip().split('\t')

		time = float(splitLine[0])
		
		if splitLine[1] == 'phosphorylate':
			totalPhosphorylation += 1
			x.append(time)
			y.append(totalPhosphorylation)
		elif splitLine[1] == 'dephosphorylate':
			totalPhosphorylation -= 1
			x.append(time)
			y.append(totalPhosphorylation)

if len(x) > 0 and plotted <= maxLines:
	plt.plot(x,y,alpha=0.5)
	plotted += 1
	totalPhosphorylation = 0
	ends.append(y[-1:])
	y = []
	x = []


plt.xlabel('Time (seconds)')
plt.ylabel('Phosphorylated Receptors')
plt.savefig('phosphorylated.pdf')

print np.mean(ends)
print np.std(ends)
