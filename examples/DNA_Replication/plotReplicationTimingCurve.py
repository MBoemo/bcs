import sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt

chrLength_kb = 814

#get the simulation data
f = open(sys.argv[1],'r')
numberOfIterations = 0

timeStore = [0.0 for x in range(0,chrLength_kb)]
for line in f:
	if line[0] == '>':
		alreadyDone = []
		numberOfIterations += 1
		continue

	splitLine = line.split('\t')

	if splitLine[2] in ["FL","FR"]:
		pos = int( splitLine[4] )
		time = float( splitLine[0] )
		if pos not in alreadyDone:
			timeStore[pos] += time
			alreadyDone.append(pos)
f.close()

for i, j in enumerate(timeStore):
	timeStore[i] = float(j) / float(numberOfIterations)

plt.figure(1)
matplotlib.rcParams.update({'font.size': 10})
x = range(chrLength_kb)
plt.plot(x[1:], timeStore[1:],'b',linewidth=3,alpha=0.7)
plt.xlabel('Position on Chromosome (kb)')
plt.ylabel('Time in S-phase (min)')
plt.title('S. Cerevisiae Chr 2 Replication Timing')
plt.gca().invert_yaxis()
plt.savefig('trep.pdf')
