# SSH into a running minix machine from a host computer
1) Boot the Minix
2) On the host machine, use this command:
    ssh -p 2222 -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa jshin53@127.0.0.1

# SCP a file into a running minix machine from a host computer
    scp -P 2222 -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa ./myfile.txt root@127.0.0.1:/root/


