# projet_far

IT Engineering project of 3rd years students at Polytech Montpellier. Messaging and file transfer client/server application written in C.

## Getting Started

### Prerequisites

- gcc
- [ optionnals (for the python lancher ``run.py``) ] :
  - Gnome Terminal
  - python3
  - python3-tk (on debian systems: ``sudo apt install python3-tk``)
  - install the requirements with ``pip3 install -r requirements.txt``
- [ recommended ] :
    - GNU/Linux system

### Compiling

Use the script ``compile.sh`` to compile the project. It will create a ``.out`` folder and put the compiled files in it.

### Running

Start the server with ``.out/server.o <port1> <port2>`` and the client with ``.out/client.o <server's address> <port1> <port2>``.
- the server's port1 is the same as the client's port1
- the server's port2 is the same as the client's port2
- port1 is used for the messaging, port2 is used for the file transfer, so don't use the same port for both
- the address of the server is the IP address of the machine hosted the server or ``127.0.0.1`` if the server is on the same machine as the client

#### Use the python launcher

You can also use the python launcher ``run.py`` to start servers and clients. It will open a new terminal for each one.

## Project structure
- .out: The compiled files of the final project
- examples: Some dev examples
- src/sprint.: The source files of the different states of the project. The final project is in ``src/sprint.4``
- files_Client: The folder that contains the files the client can send to the server
- files_Server: The folder that contains the files the server can send to the clients
- run.py: The python launcher
- channels.txt: The file that contains the channels of the server
- manual.txt: The file that contains the list of commands usable by the client
