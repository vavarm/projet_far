mkdir .out
rm .out/*.o
gcc -o .out/server.o src/sprint2/server.c -lpthread
gcc -o .out/client.o src/sprint2/client.c -lpthread