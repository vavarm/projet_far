mkdir .out
rm .out/*.o
gcc -o .out/server.o src/sprint4/server.c -lpthread
gcc -o .out/client.o src/sprint4/client.c -lpthread