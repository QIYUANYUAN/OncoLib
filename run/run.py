import sys
sys.path.insert(0, '../build/')

import oncolib
import glob

finished = [filename.lstrip("output/T_").rstrip(".txt") for filename in glob.glob("output/T_*.txt")]
#exclude = open("excluded.txt");
#excluded = [line.rstrip('\n') for line in exclude]

for filename in glob.glob("input/F_*.tsv"):
	ind=filename.lstrip("input/F_").rstrip("*.tsv");
	if ind in finished:
		print "finished %s, skipped"%ind;
		continue;
	print "running", ind
	oncolib.enumerate(filename,"output/T_%s.txt"%ind,True,1,-1,False)
