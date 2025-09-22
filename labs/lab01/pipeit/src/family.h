// executes the '$ sort -r' command on the incoming stream from pipe 
// ipc_fd and writes it to the file in the project directory called 
// "outfile".
void child2(int *ipc_fd);


// exec the '$ ls' command
// toss that output through the pipe ipc_fd for child 2 to recieve
void child1(int *ipc_fd);
