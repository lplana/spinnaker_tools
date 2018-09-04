import glob, os

files = glob.glob(os.getcwd()+"/*.txt")

output_sizes = [1, 2, 4, 8, 16, 32, 64, 128, 255]
cores = range(1,18)

output_dict = {}
for i in output_sizes:
    output_dict[i] = []

core_output_dict = {}
for i in cores:
    core_output_dict[i] = []

for i in cores:
    size = []
    avg = [] 
    min = []
    max = []
    file = open(os.getcwd()+"/"+str(i)+".txt")
    for line in file.readlines():
            if line.startswith("size"):
                size.append(int(line.split("size: ")[1].split(" avg:")[0]))
                avg.append(int(line.split("avg:")[1].split(" ns [")[0]))
                min.append(int(line.split(" ns [")[1].split(" ns :")[0]))
                max.append(int(line.split(" ns :")[1].split(" ns]")[0]))

    core_output_dict[i] = [size, avg, min, max]
    print i, core_output_dict[i][1], core_output_dict[i][2], core_output_dict[i][3]

    

print_size_averaged=False
if print_size_averaged:
    for f in files:
        file = open(f)
        for line in file.readlines():
            if line.startswith("size"):
	        size = line.split("size: ")[1].split(" avg:")[0]
                avg = line.split("avg:")[1].split(" ns [")[0]
                min = line.split(" ns [")[1].split(" ns :")[0]
                max = line.split(" ns :")[1].split(" ns]")[0]
    
                output_dict[int(size)].append([avg, min, max])
    
    print "\nSize, Avg, Min Max"
    for i in output_sizes:
        avg_list = []
        min_list = []
        max_list = []
        for j in output_dict[i]:
	    print i, int(j[0]), int(j[1]), int(j[2])
            avg_list.append(int(j[0]))
            min_list.append(int(j[1]))
            max_list.append(int(j[2]))
        print "\naverages"
        print i, np.mean(avg_list), np.mean(min_list), np.mean(max_list)
        print "\n"



