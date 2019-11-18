import sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt

x = []
y = []
startCycle = 0.0
cycle = 0.0
plt.figure()
maxCount = 25
plotCount = 0

f = open(sys.argv[1], 'r')
for line in f:
	
	if line[0] == '>':

		if len(x) > 0 and len(y) > 0:

			plotCount += 1
			if plotCount < maxCount:
				plt.plot(x,y,alpha=0.5)
			else:
				break

		x = [0]
		y = [0]

		cellCount = 1
		adaCount = 0

		continue

	splitLine = line.rstrip().split('\t')
	time = float(splitLine[0])


	if splitLine[1] == "generate_Ada":
		adaCount += 1
		x.append(time)
		y.append(float(adaCount)/float(cellCount))
	elif splitLine[1] == "finish":
		cellCount += 1


f.close()

plt.xlabel('Time (minutes)')
plt.ylabel('Ada Molecules per Cell')
plt.xlim(0,250)
plt.savefig('population.pdf')
