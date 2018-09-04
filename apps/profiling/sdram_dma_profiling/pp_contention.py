import glob, os
import numpy
import matplotlib.pyplot as plt
from matplotlib import rcParams
import matplotlib 

files = glob.glob(os.getcwd()+"/*.txt")

#output_sizes = [1, 2, 4, 8, 16, 32, 64, 128, 255]
output_sizes = [12, 25, 50, 75, 100, 125, 150, 175, 200, 225, 250]
cores_contending = [1, 2, 4, 8, 12, 16]

output_dict = {}
for i in cores_contending:
    output_dict[i] = []

for i in reversed(cores_contending):
    size = []
    avg = [] 
    min = []
    max = []
    file = open(os.getcwd()+"/1_"+str(i)+"_cont.txt")
    for line in file.readlines():
            if line.startswith("size"):
                size.append(int(line.split("size: ")[1].split(" avg:")[0]))
                avg.append(int(line.split("avg:")[1].split(" ns [")[0]))
                min.append(int(line.split(" ns [")[1].split(" ns :")[0]))
                max.append(int(line.split(" ns :")[1].split(" ns]")[0]))

    output_dict[i] = max

for i in output_dict.keys():
    print i, output_dict[i]
   
contention_contour = numpy.zeros((len(cores_contending), len(output_sizes)))
for i in reversed(range(len(cores_contending))):
    for j in range(len(output_dict[cores_contending[i]])):
        print i, j
        contention_contour[len(cores_contending) - 1 - i ][j] = float(output_dict[cores_contending[i]][j])/1000

print contention_contour

plt.figure(figsize=(10, 4))
rcParams['font.family'] = 'Times New Roman'
rcParams['font.size'] = '16'
plt.xlabel("DMA Size (words)")
plt.xticks(range(len(output_sizes)), output_sizes)
plt.ylabel("Cores Contending")
plt.yticks(range(len(cores_contending)), reversed(cores_contending))
plt.imshow(contention_contour, cmap='viridis', interpolation='nearest', aspect='auto')
plt.clim(0,25)
cbar = plt.colorbar()
normi = matplotlib.colors.Normalize(vmin=1, vmax=10);
cbar.set_norm(normi)
cbar.set_label('DMA Time ($\mu s$)')
plt.tight_layout()
plt.savefig("contention_profiling.pdf", dpi=2000, format='pdf')
plt.show()

 
