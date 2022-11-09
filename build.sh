#!/bin/sh
clang ./src/sender/tcpserver.c -o ./out/sender/tcpserver
clang ./src/receiver/tcpclient.c -o ./out/receiver/tcpclient