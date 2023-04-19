mkdir .out
rm .out/*.o
gcc -o .out/server.o src/sprint1/seance2-3-4/v1/server.c -lpthread
gcc -o .out/client.o src/sprint1/seance2-3-4/v1/client.c -lpthread