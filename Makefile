build: lab3a.cpp
	g++ -Wall -Wextra -g -o lab3a lab3a.cpp
dist: build
	tar -czvf lab3a-304925920.tar.gz lab3a.cpp ext2_fs.h README Makefile
clean:
	rm -f lab3a lab3a-304925920.tar.gz
