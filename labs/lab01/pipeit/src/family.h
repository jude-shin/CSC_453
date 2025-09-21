/*
 *Parameters: 
 *  ipc_fd: A Pointer to the pipe which child1 and child2 will communicate. 
 *  Note that ipc_fd[0] and ipc_fd[1] represent the 'read' end 'write' end of 
 *  the pipe respectively.
 *Function:
 *Executes the '$ sort -r' command on the incoming stream from pipe ipc_fd
 *  and writes it's output to a file in the project directory called
 *  'outfile'. If that file is not found, a file called 'outfile' is created
 *  and used.
 * Returns:
 *   void.
 */
void child2(int *ipc_fd);

/*
 *Parameters: 
 *  ipc_fd: A Pointer to the pipe which child1 and child2 will communicate. 
 *  Note that ipc_fd[0] and ipc_fd[1] represent the 'read' end 'write' end of 
 *  the pipe respectively.
 *Function:
 *  Executes the '$ ls' command and tosses that output through the pipe ipc_fd 
 *  for child 2.
 *Returns:
 *  void.
 */
void child1(int *ipc_fd);
