fi=open("Input.tsv")
lines=[line for line in fi if len(line)>0]
lines=lines[1:]
fc=open("clusters.tsv")
clusters={line.split('\t')[0]:line.split('\t')[1].rstrip('\n').split(',') for line in fc}

data={ i :[ [] for j in range(len(clusters[i])) ] for i in (clusters) }



#		fo.write("%d\tR%d\t0\tP\t0\trt\t1\t1\n"%(_,_+1));
#	fo.close()




def handle(line):
	et=line.split('\t')
	ind=et[0]
	if et[0] in clusters and et[1] in clusters[et[0]]:
		sp=et[2].split(';');
		i_cc=clusters[et[0]].index(et[1]);
		if len(data[ind][i_cc])==0:
			data[ind][i_cc]=[[] for s in sp];
		for i in range(len(sp)):
			s=sp[i];
			data[ind][i_cc][i].append(min(1.0,float(s.split(':')[1])))
			#if et[0]=='CRUK0001':
			#	print s,et[1], int(s.split(':')[1].split('/')[0]), int(s.split(':')[1].split('/')[1])
			#	print(data[ind][i_cc][i])


for line in lines:
	handle(line)

def percent(List):
	List.sort()
	L=len(List)
	l=int(L*0.05)
	return List[l],List[-l-1];

for ind in clusters:
	fo=open("F/F_%s.tsv"%ind,"a");
	fo.write("1 #anatomical sites\n");
	sp=len(data[ind][0])
	fo.write("%d #samples\n"%(sp));
	cc=len(clusters[ind])
	if (ind)==("CRUK0001"):
		print cc,clusters[ind];
	fo.write("%d #characters\n"%(cc))
	fo.write("#sample_index\tsample_label\tanatomical_site_index\tanatomical_site_label\tcharacter_index\tcharacter_label\tf-\tf+\n");
	for i in range(sp):
		for j in range(cc):
			a,b=percent(data[ind][j][i])
			fo.write("%d\tR%d\t0\tP\t%d\tv%d\t%f\t%f\n"%(i,i+1,j,j+1,a,b));
	fo.close()
