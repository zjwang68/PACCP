CC = gcc
OBJ = server.o 

server : $(OBJ)
	@$(CC) -lm -pthread -o $@ $^
	@echo '"'server'"' is the excutable file
server.o : server.c  
	@$(CC) -g -c server.c
.PHONY : clean
clean:
	rm -f server $(OBJ)
