// atomizer - A trivial sexpr parser for configuration files.
//
// Christopher Oliver - November 24, 2012
#define NIL 0
#define ATOM 1
#define CONS 2

#define IS_NIL(CELL) ((CELL)->type == NIL)
#define IS_CONS(CELL) ((CELL)->type == CONS)
#define IS_ATOM(CELL) ((CELL)->type == ATOM)
#define DYNAMIC_FLAG(CELL) ((CELL)->value.atom.is_dynamic)
#define ATOM_NAME(CELL) ((CELL)->value.atom.name)
#define ATOM_NUMBER(CELL) ((CELL)->value.atom.number)
#define ATOM_LINE(CELL) ((CELL)->value.atom.line)
#define ATOM_DATA(CELL) ((CELL)->value.atom.data)
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
    struct {
      int line;
      const char *name;
      int number;
      int is_dynamic;
      void *data;
    } atom;
  } value;
};

struct cell *read_cfg_file(const char *filename);
char *read_cfg_error(void);
void free_cfg(struct cell *);
void free_symbols();
unsigned int intern_string(const char **);
struct cell *assoc_key(struct cell *list, unsigned int);
void alloc_dynamic_atom_data(struct cell *atom, size_t size);
void set_static_atom_data(struct cell *atom, void *data);
