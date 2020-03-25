import sys
import math
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
import numpy as np


f = open(sys.argv[1],'r')
A_store = []
AB_store = []
Cuu_store = []
Cpu_store = []
Cpp_store = []
t = []

for line in f:

        if line[0] == '>':
                prevTime = 0
                A = 100
                AB = 0
                Cuu = 1000
                Cpu = 0
                Cpp = 0
                continue

        splitLine = line.rstrip().split('\t')
        time = int(math.floor(float(splitLine[0])))

        if time > 250:
                continue


        A_store.append(A)
        AB_store.append(AB)
        Cuu_store.append(Cuu)
        Cpu_store.append(Cpu)
        Cpp_store.append(Cpp)
        t.append(time)


        if splitLine[1] == 'bindA' and splitLine[2] == 'B':
                AB+=1
                A -= 1
        elif splitLine[1] == 'unbindA' and splitLine[2] == 'B':
                AB-=1
                A += 1
        elif splitLine[1] == 'bindA' and splitLine[2] == 'C':
                A -= 1
        elif splitLine[1] == 'bindAB' and splitLine[2] == 'C':
                AB -= 1
        elif splitLine[1] == 'phos_x1_AB':
                Cuu -= 1
                Cpu += 1
                AB += 1
        elif splitLine[1] == 'phos_x1_A':
                Cuu -= 1
                Cpu += 1
                A += 1
        elif splitLine[1] == 'phos_x2':
                Cpu -= 1
                Cpp += 1
                A += 1

plt.figure(1)
plt.plot(t,A_store,label='A')
plt.plot(t,AB_store,label='AB')
plt.plot(t,Cuu_store,label='Cuu')
plt.plot(t,Cpu_store,label='Cpu')
plt.plot(t,Cpp_store,label='Cpp')
plt.xlabel('Time (seconds)')
plt.ylabel('Count')
plt.ylim(0,1000)
plt.legend()
plt.savefig('ABC.pdf')
