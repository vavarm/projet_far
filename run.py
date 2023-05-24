# 1) install the dependencies:

#   sudo apt-get install python3-tk

#   pip install -r requirements.txt

# 2) Run the program:

#   python3 run.py


import tkinter as tk

from tkinter import ttk

import subprocess

import platform

from platform import uname


# function to launch the server


def launchServer(port1, port2):
    # create a bash command to run the server

    command = ".out/server.o " + port1 + " " + port2

    print(command)

    # run the bash command

    # Ubuntu

    if "ubuntu" in platform.version().lower():
        process = subprocess.Popen(
            "gnome-terminal -- bash -c '" + command + "; exec bash'", shell=True
        )

    else:
        print("OS not supported")


# function to launch the client


def launchClient(ip, port1, port2):
    # create a bash command to run the client

    command = ".out/client.o " + ip + " " + port1 + " " + port2

    print(command)

    # run the bash command

    # Ubuntu

    if "ubuntu" in platform.version().lower():
        process = subprocess.Popen(
            "gnome-terminal -- bash -c '" + command + "; exec bash'", shell=True
        )

    else:
        print("OS not supported")


# function to open a new window with two text fields and a button


def openServerWindow():
    window = tk.Tk()

    window.title("Connect server")

    window.geometry("300x100")

    window.configure(bg="white")

    # create one label for the port

    portLabel = tk.Label(window, text="Port1:")

    portLabel.place(x=10, y=10)

    portLabel2 = tk.Label(window, text="Port2:")

    portLabel2.place(x=10, y=40)

    # create one text field for the port

    port = tk.Entry(window)

    port.place(x=100, y=10)

    port.insert(0, "3000")

    port2 = tk.Entry(window)

    port2.place(x=100, y=40)

    port2.insert(0, "4000")

    # create a button

    connect = tk.Button(
        window, text="Connect", command=lambda: launchServer(port.get(), port2.get())
    )

    connect.place(x=200, y=70)

    window.mainloop()


# function to open a new window with two text fields and a button


def openClientWindow():
    window = tk.Tk()

    window.title("Connect client")

    window.geometry("300x200")

    window.configure(bg="white")

    # create two labels

    ipLabel = tk.Label(window, text="IP:")

    ipLabel.place(x=10, y=10)

    portLabel1 = tk.Label(window, text="Port1:")

    portLabel1.place(x=10, y=40)

    portLabel2 = tk.Label(window, text="Port2:")

    portLabel2.place(x=10, y=70)

    # create two text fields

    ip = tk.Entry(window)

    ip.place(x=100, y=10)

    port1 = tk.Entry(window)

    port1.place(x=100, y=40)

    ip.insert(0, "127.0.0.1")

    port1.insert(0, "3000")

    port2 = tk.Entry(window)

    port2.place(x=100, y=70)

    port2.insert(0, "4000")

    # create a button

    connect = tk.Button(
        window, text="Connect", command=lambda: launchClient(ip.get(), port1.get(), port2.get())
    )

    connect.place(x=200, y=100)

    window.mainloop()


# create a window

window = tk.Tk()

window.title("Server/Client")

window.geometry("300x100")

window.configure(bg="white")


# create two buttons

server = tk.Button(window, text="Server", command=openServerWindow)

server.place(x=10, y=10)

client = tk.Button(window, text="Client", command=openClientWindow)

client.place(x=150, y=10)


window.mainloop()
