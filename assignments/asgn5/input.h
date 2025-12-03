#ifndef INPUT 
#define INPUT 

/* Parses the flags given. Exits with EXIT_FAILURE if anything goes wrong. */
int parse_flags(int argc, char* argv[], bool* verb, int* p_part, int* s_part);

/* Parses the rest of the input for minls, setting the caller values. */
int parse_minls_input(
    int argc, 
    char* argv[], 
    char** img,
    char** pth,
    int i);

/* Parses the rest of the input for minget, setting the caller values. */
int parse_minget_input(
    int argc, 
    char* argv[], 
    char** img,
    char** s_pth, 
    char** d_pth,
    int i);

/* Safely parses an argument that is supposed to be a positive integer. Returns
   -1 if something goes wrong. */
int parse_positive_int(char* s);

#endif
