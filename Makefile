all: assn_4

assn_4: assn_4.c
	gcc -g -o assn_4 assn_4.c -lm
tmp: tmp.c
	gcc -g -o tmp tmp.c -lm
rm: index.bin
	rm index.bin tmp
clean:
	rm assn_4
full_clean:
	rm assn_4 index.bin
