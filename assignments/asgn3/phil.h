
char get_label(int i);

typedef struct Phil {
  char name;   // Ascii value of this philosopher's name
  int doing;   // What the philosopher is doing (thinking, eating, or changing)
  int wants;   // What the philosopher currently wants (thinky, hungry)
  Phil *right; // The philosopher sitting to the right
  Phil *left;  // The philosopher sitting to the left 
} Phil;
