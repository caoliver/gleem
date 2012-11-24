#define NIL 0
#define ATOM 1
#define CONS 2

#define IS_NIL(CELL) ((CELL)->type == NIL)
#define IS_CONS(CELL) ((CELL)->type == CONS)
#define IS_ATOM(CELL) ((CELL)->type == ATOM)
#define ATOM_NAME(CELL) ((CELL)->value.atom)
#define CELL_TYPE(CELL) ((CELL)->type)
#define CAR(CELL) ((CELL)->value.cons.car)
#define CDR(CELL) ((CELL)->value.cons.cdr)
#define DEFINE_NIL(NIL_NAME)					\
  static struct cell NIL_NAME =					\
    { .type=NIL, .value.cons = {&NIL_NAME, &NIL_NAME} }

struct cell {
  int type;
  union {
    struct {
      struct cell *car, *cdr;
    } cons;
    const char *atom;
  } value;
};

struct cell *read_cfg_file(const char *filename);
char *read_cfg_error(void);
void free_cfg(struct cell *);
void free_symbols();
const char *intern_string(const char *);
