import matplotlib
from matplotlib import pyplot as plt
import numpy as np

E = [10,25,50,100,250,500,1000]
F = [100,100,100,100,100,100,100]
Mean = [4,6.48,14.96,97,189.83,192.9,194.12]
Stdv = [2.0,2.62,4.05,18.72,4.4,3.57,1.79]

x = []
y = np.array(Mean)/200
Stdv = np.array(Stdv)/200
for i, e in enumerate(E):
	x.append(np.log10(float(e)/float(100)))
plt.figure()
plt.errorbar(x,y,yerr=Stdv,barsabove=True)
plt.xlabel('log10([Kinase]/[Phosphatase])')
plt.ylabel('Phosphorylated Receptors (Fraction)')
plt.savefig('sigmoid.pdf')
