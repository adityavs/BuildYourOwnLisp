#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "../mpc/mpc.h"

/* Forward Declarations */

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Lisp Value */

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
  int type;

  /* Basic */
  long num;
  char* err;
  char* sym;
  
  /* Function */
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  
  /* Expression */
  int count;
  lval** cell;
};

lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;  
  va_list va;
  va_start(va, fmt);  
  v->err = malloc(512);  
  vsnprintf(v->err, 511, fmt, va);  
  v->err = realloc(v->err, strlen(v->err)+1);
  va_end(va);  
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_builtin(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

/* Forward Declarations for environment functions */
lenv* lenv_new(void);

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  
  /* Set Builtin to Null */
  v->builtin = NULL;
  
  /* Built new environment */
  v->env = lenv_new();
  
  /* Set Formals and Body */
  v->formals = formals;
  v->body = body;
  return v;  
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lenv_del(lenv* e);

void lval_del(lval* v) {

  switch (v->type) {
    case LVAL_NUM: break;
    case LVAL_FUN: 
      if (v->builtin) {
        /* Do Nothing */
      } else {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
    break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
    break;
  }
  
  free(v);
}

lenv* lenv_copy(lenv* e);

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;
  switch (v->type) {
    case LVAL_FUN:
      if (v->builtin) {
        x->builtin = v->builtin;
      } else {
        x->env = lenv_copy(v->env);
        x->formals = lval_copy(v->formals);
        x->body = lval_copy(v->body);
      }
    break;
    case LVAL_NUM: x->num = v->num; break;
    case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
    case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
    break;
  }
  return x;
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_join(lval* x, lval* y) {  
  for (int i = 0; i < y->count; i++) {
    x = lval_add(x, y->cell[i]);
  }
  free(y->cell);
  free(y);  
  return x;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];  
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));  
  v->count--;  
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

void lval_print(lval* v);

void lval_print_expr(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);    
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_FUN:
      if (v->builtin) {
        printf("<builtin>");
      } else {
        printf("(\\ "); lval_print(v->formals); putchar(' '); lval_print(v->body); putchar(')');
      }
    break;
    case LVAL_NUM:   printf("%li", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_print_expr(v, '(', ')'); break;
    case LVAL_QEXPR: lval_print_expr(v, '{', '}'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

/* Lisp Environment */

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->par = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }  
  free(e->syms);
  free(e->vals);
  free(e);
}

lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);
  for (int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }
  return n;
}

lval* lenv_get(lenv* e, lval* k) {
  
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) { return lval_copy(e->vals[i]); }
  }
  
  /* If no symbol check in parent otherwise error */
  if (e->par) {
    return lenv_get(e->par, k);
  } else {
    return lval_err("Unbound Symbol '%s'", k->sym);
  }
}

void lenv_put(lenv* e, lval* k, lval* v) {
  
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      e->syms[i] = realloc(e->syms[i], strlen(k->sym)+1);
      strcpy(e->syms[i], k->sym);
      return;
    }
  }

  
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);  
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v) {
  /* Iterate till e has no parent */
  while (e->par) { e = e->par; }
  /* Put value in e */
  lenv_put(e, k, v);
}

/* Builtins */

#define LASSERT(args, cond, fmt, ...) if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

lval* lval_eval(lenv* e, lval* v);

lval* builtin_lambda(lenv* e, lval* a) {
  /* Check Two arguments, each of which are Q-Expressions */
  LASSERT(a, (a->count == 2                 ), "Lambda passed too many arguments. Got %i, Expected %i", a->count, 2);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Lambda passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  LASSERT(a, (a->cell[1]->type == LVAL_QEXPR), "Lambda passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[1]->type), ltype_name(LVAL_QEXPR));
  
  /* Check first Q-Expression contains only Symbols */
  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM), "Lambda cannot define non-symbol. Got %s, Expected %s.", ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }
  
  /* Pop first two arguments and pass them to lval_lambda */
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);
  
  return lval_lambda(formals, body);
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1                 ), "Function 'head' passed too many arguments. Got %i, Expected %i.", a->count, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'head' passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  LASSERT(a, (a->cell[0]->count != 0        ), "Function 'head' passed {}.");
  
  lval* v = lval_take(a, 0);  
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1                 ), "Function 'tail' passed too many arguments. Got %i, Expected %i.", a->count, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'tail' passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  LASSERT(a, (a->cell[0]->count != 0        ), "Function 'tail' passed {}.");

  lval* v = lval_take(a, 0);  
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT(a, (a->count == 1                 ), "Function 'eval' passed too many arguments. Got %i, Expected %i.", a->count, 1);
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'eval' passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  
  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

#define LFASSERT(a0, a1, cond, fmt, ...) if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(a0); lval_del(a1); x = err; break; }

lval* builtin_join(lenv* e, lval* a) {
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function 'join' passed incorrect type. Got %s, Expected %s.", ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  lval* x = lval_pop(a, 0);
  
  while (a->count) {
    lval* y = lval_pop(a, 0);
    LFASSERT(x, y, (x->type == LVAL_QEXPR), "Function 'join' passed incorrect type. Got %s, Expected %s.", ltype_name(x->type), ltype_name(LVAL_QEXPR));
    LFASSERT(x, y, (y->type == LVAL_QEXPR), "Function 'join' passed incorrect type. Got %s, Expected %s.", ltype_name(y->type), ltype_name(LVAL_QEXPR));
    x = lval_join(x, y);
  }
  
  lval_del(a);
  return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
  LASSERT(a, (a->cell[0]->type == LVAL_NUM), "Function '%s' passed incorrect type. Got %s, Expected %s.", op, ltype_name(a->cell[0]->type), ltype_name(LVAL_NUM));
  
  lval* x = lval_pop(a, 0);
  
  while (a->count > 0) {  
    lval* y = lval_pop(a, 0);
    
    LFASSERT(x, y, (x->type == LVAL_NUM), "Function '%s' passed incorrect type. Got %s, Expected %s.", op, ltype_name(x->type), ltype_name(LVAL_NUM));
    LFASSERT(x, y, (y->type == LVAL_NUM), "Function '%s' passed incorrect type. Got %s, Expected %s.", op, ltype_name(y->type), ltype_name(LVAL_NUM));
    
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      LFASSERT(x, y, (y->num != 0), "Division By Zero.");
      x->num /= y->num;
    }
    
    lval_del(y);
  }
  
  lval_del(a);
  return x;
}

lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT(a, (a->cell[0]->type == LVAL_QEXPR), "Function '%s' passed incorrect type. Got %s, Expected %s.", func, ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));
  
  lval* syms = a->cell[0];  
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->type == LVAL_SYM), "Function '%s' cannot define non-symbol. Got %s, Expected %s.", func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
  }
  
  LASSERT(a, (syms->count == a->count-1), "Function '%s' passed too many arguments for symbols. Got %i, Expected %i.", func, syms->count, a->count-1);
  
  for (int i = 0; i < syms->count; i++) {
    /* If 'def' define in global scope. If 'put' define in local scope */
    if (strcmp(func, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); }
    if (strcmp(func, "put") == 0) { lenv_put(e, syms->cell[i], a->cell[i+1]); } 
  }
  
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "put"); }

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_builtin(func);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* Variable Functions */
  lenv_add_builtin(e, "\\",  builtin_lambda); 
  lenv_add_builtin(e, "def", builtin_def); lenv_add_builtin(e, "=", builtin_put);
  
  /* List Functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head); lenv_add_builtin(e, "tail",  builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval); lenv_add_builtin(e, "join",  builtin_join);
  
  /* Mathematical Functions */
  lenv_add_builtin(e, "+",    builtin_add); lenv_add_builtin(e, "-",     builtin_sub);
  lenv_add_builtin(e, "*",    builtin_mul); lenv_add_builtin(e, "/",     builtin_div);
}

/* Evaluation */

lval* lval_apply(lenv* e, lval* f, lval* a) {
  
  /* If Builtin then simply apply that */
  if (f->builtin) { return f->builtin(e, a); }
  
  /* Record Argument Counts */
  int given = a->count;
  int total = f->formals->count;
  
  /* While arguments still remain to be processed */
  while (a->count) {
    
    /* If we've ran out of formal arguments to bind */
    if (f->formals->count == 0) {
      lval_del(a); return lval_err("Function passed too many arguments. Got %i, Expected %i.", given, total); 
    }
    
    /* Pop the first symbol from the formals */
    lval* sym = lval_pop(f->formals, 0);
    
    /* Special Case to deal with '&' */
    if (strcmp(sym->sym, "&") == 0) {
      
      /* Ensure '&' is followed by another symbol */
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
      }
      
      /* Next formal should be bound to remaining arguments */
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym); lval_del(nsym);
      break;
    }
    
    /* Pop the next argument from the list */
    lval* val = lval_pop(a, 0);
    
    /* Bind a copy into the function's environment */
    lenv_put(f->env, sym, val);
    
    /* Delete symbol and value */
    lval_del(sym); lval_del(val);
  }
  
  /* Argument list is now bound so can be cleaned up */
  lval_del(a);
  
  /* If '&' remains in formal list it should be bound to empty list */
  if (f->formals->count > 0 &&
    strcmp(f->formals->cell[0]->sym, "&") == 0) {
    
    /* Check to ensure that & is not passed invalidly. */
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. Symbol '&' not followed by single symbol.");
    }
    
    /* Pop and delete '&' symbol */
    lval_del(lval_pop(f->formals, 0));
    
    /* Pop next symbol and create empty list */
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();
    
    /* Bind to environment and delete */
    lenv_put(f->env, sym, val);
    lval_del(sym); lval_del(val);
  }
  
  /* If all formals have been bound evaluate */
  if (f->formals->count == 0) {
  
    /* Set Function Environment parent to current evaluation Environment */
    f->env->par = e;
    
    /* Evaluate and return */
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    /* Otherwise return partially evaluated function */
    return lval_copy(f);
  }
  
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  
  for (int i = 0; i < v->count; i++) { v->cell[i] = lval_eval(e, v->cell[i]); }
  for (int i = 0; i < v->count; i++) { if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); } }
  
  if (v->count == 0) { return v; }  
  if (v->count == 1) { return lval_eval(e, lval_take(v, 0)); }
  
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) { lval* err = lval_err("S-Expression starts with incorrect type. Got %s, Expected %s.", ltype_name(f->type), ltype_name(LVAL_FUN)); lval_del(f); lval_del(v); return err; }
  
  lval* result = lval_apply(e, f, v);
  lval_del(f);
  return result;
}

lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM)   { return lenv_get(e, v); }
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  return v;
}

/* Reading */

lval* lval_read_num(mpc_ast_t* t) {
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("Invalid Number.");
}

lval* lval_read(mpc_ast_t* t) {
  
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }
  
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  
  return x;
}

/* Main */

static char input[2048];

int main(int argc, char** argv) {
  
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispy  = mpc_new("lispy");
  
  mpca_lang(
    "                                                     \
      number : /-?[0-9]+/ ;                               \
      symbol : /[a-zA-Z_\\+\\-\\*\\/\\\\]+/ ;             \
      sexpr  : '(' <expr>* ')' ;                          \
      qexpr  : '{' <expr>* '}' ;                          \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ;  \
      lispy  : /^/ <expr>* /$/ ;                          \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
  mpc_print(Symbol);
  
  fputs("Lispy Version 0.0.0.0.4\n", stdout);
  fputs("Press Ctrl+c to Exit\n\n", stdout);
  
  lenv* e = lenv_new();
  lenv_add_builtins(e);
  
  while (1) {
  
    fputs("lispy> ", stdout);
    fgets(input, 2047, stdin);
    
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      
      lval* x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      lval_del(x);
      
      mpc_ast_delete(r.output);
    } else {    
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
  }
  
  lenv_del(e);
  
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
  
  return 0;
}
