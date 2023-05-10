mkdir .out
rm .out/*.o
gcc -o .out/server.o src/sprint3/server.c -lpthread
gcc -o .out/client.o src/sprint3/client.c -lpthread