/*
    A multithreaded server is a server that can handle multiple clients at the same time.

    instance(void* dsClient){
        while(1)
            on:
                receive data from client
            then:
                send data to client
    }

    main(){
        create a TCP socket
        bind the socket to a port
        while(1)
            listen for connections
                on:
                    accept a connection -> dsClient
                then:
                    create a thread to handle the client: pthread_create(&thread, NULL, instance, dsClient)
                    start the thread: pthread_start(thread)
    }

*/