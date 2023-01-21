nscan: main.c 
	gcc main.c -o nscan -lpthread -lm -lresolv

clean:
	rm -f *.o *~ main *~ nscan