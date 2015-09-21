all: myproject myproject2

myproject: myproject.c
	gcc -g myproject.c -o myproject
	mv myproject /etc/wyliodrin/build/myproject_id

myproject2: myproject2.c
	gcc -g myproject2.c -o myproject2
	mv myproject2 /etc/wyliodrin/build/myproject2_id

clean:
	rm -rf myproject myproject2 out.log err.log
