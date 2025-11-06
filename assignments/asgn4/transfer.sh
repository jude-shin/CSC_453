#!/usr/bin/sh

scp -r -P 2222 -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa ./secret/ jshin53@127.0.0.1:/home/jshin53/asgn4

