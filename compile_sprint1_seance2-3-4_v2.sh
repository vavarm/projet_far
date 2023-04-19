mkdir .out
rm .out/*.o
gcc -o .out/server.o src/sprint1/seance2-3-4/v2/server.c -lpthread
gcc -o .out/client.o src/sprint1/seance2-3-4/v2/client.c -lpthread