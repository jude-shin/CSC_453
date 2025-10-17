
typedef struct PhilContext {
  // Ascii value of this philosopher's name
  char name;
  // The philosopher can either be thinking, eating, or changing
  int doing;
  // The philosopher currently wants
  int wants;
} Phil;
