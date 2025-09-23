
// hold the data structure for the linked list in here. this should hold some
// pointer information, size, whether it is free or being used, and possibly
// the previous chunk as well

// you should name it chunks

// 64k chunks

// chunk is a chunk of memory that my custom malloc is going to be allocating
// pointer to the next and previous chunk, if it is being used, and it's size
struct chunk {
  long unsigned int size;
  bool is_available;
  chunk *prev;
  chunk *next;
};

