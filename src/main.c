#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clan/clan.h>
#include <osl/osl.h>

#include <isl/ctx.h>
#include <isl/mat.h>
#include <isl/printer.h>
#include <isl/set.h>
#include <isl/space.h>
#include <isl/val.h>

#include <barvinok/isl.h>

struct param_values {
  char *name;
  int value;
  struct param_values *next;
};

typedef struct param_values param_values_t;

/* Convert a osl_relation_p containing the constraints of a domain
 * to an isl_set.
 * One shot only; does not take into account the next ptr.
 */
static __isl_give isl_set *osl_relation_to_isl_set(osl_relation_p relation,
                                                   __isl_take isl_space *dim) {
  int i, j;
  int n_eq = 0, n_ineq = 0;
  isl_ctx *ctx;
  isl_mat *eq, *ineq;
  isl_basic_set *bset;

  ctx = isl_space_get_ctx(dim);

  for (i = 0; i < relation->nb_rows; ++i)
    if (osl_int_zero(relation->precision, relation->m[i][0]))
      n_eq++;
    else
      n_ineq++;

  eq = isl_mat_alloc(ctx, n_eq, relation->nb_columns - 1);
  ineq = isl_mat_alloc(ctx, n_ineq, relation->nb_columns - 1);

  n_eq = n_ineq = 0;
  for (i = 0; i < relation->nb_rows; ++i) {
    isl_mat **m;
    int row;

    if (osl_int_zero(relation->precision, relation->m[i][0])) {
      m = &eq;
      row = n_eq++;
    } else {
      m = &ineq;
      row = n_ineq++;
    }

    for (j = 0; j < relation->nb_columns - 1; ++j) {
      int t = osl_int_get_si(relation->precision, relation->m[i][1 + j]);
      *m = isl_mat_set_element_si(*m, row, j, t);
    }
  }

  bset = isl_basic_set_from_constraint_matrices(
      dim, eq, ineq, isl_dim_set, isl_dim_div, isl_dim_param, isl_dim_cst);
  return isl_set_from_basic_set(bset);
}

int compute_flops_in_expr(const char *expr) {
  int flops = 0;
  /*
    expr should look like
    "a[i][j][...] = a[i][j][...] + b[i+1][j] * (c[i][j] - d[i][j]) / e[i][j];"
  */

  /* first we ignore everything before the first '='*/
  while (*expr != '=' && *expr != '\0') {
    expr++;
  }
  if (*expr == '=') {
    expr++; // skip '='
  }
  /* Now count the number of +, -, *, / operators that are not inside brackets
   */
  int bracket_level = 0;
  for (const char *p = expr; *p != '\0'; p++) {
    if (*p == '[') {
      bracket_level++;
    } else if (*p == ']') {
      bracket_level--;
    } else if (bracket_level == 0) {
      if (*p == '+' || *p == '-') {
        flops += 1;
      } else if (*p == '*' || *p == '/') {
        flops += 1;
      }
    }
  }
  return flops;
}

__isl_give isl_pw_qpolynomial *
compute_ehrhart_polynomial(isl_ctx *ctx, osl_scop_p scop,
                           osl_statement_p stmt) {
  osl_relation_p osl_domain = stmt->domain;
  int precision = osl_domain->precision;

  int niter = osl_statement_get_nb_iterators(stmt);
  int nparam = osl_scop_get_nb_parameters(scop);

  isl_space *space = isl_space_set_alloc(ctx, nparam, niter);
  if (nparam > 0) {
    osl_strings_p names = scop->parameters->data;
    for (int i = 0; i < nparam; i++) {
      space = isl_space_set_dim_name(space, isl_dim_param, i, names->string[i]);
    }
  }
  if (niter > 0) {
    osl_body_p stmt_body = osl_generic_lookup(stmt->extension, OSL_URI_BODY);
    if (stmt_body != NULL) {
      for (int i = 0; i < niter; i++) {
        space = isl_space_set_dim_name(space, isl_dim_set, i,
                                       stmt_body->iterators->string[i]);
      }
    }
  }

  isl_set *set = osl_relation_to_isl_set(stmt->domain, space);
  return isl_set_card(set);
}

int evaluate_ehrhart_polynomial(isl_pw_qpolynomial *poly,
                                param_values_t *params) {
  if (poly == NULL) {
    return -1; // Error
  }

  isl_space *space = isl_pw_qpolynomial_get_space(poly);
  isl_point *point = isl_point_zero(isl_space_params(isl_space_copy(space)));

  for (param_values_t *pv = params; pv != NULL; pv = pv->next) {
    int pos = isl_space_find_dim_by_name(space, isl_dim_param, pv->name);
    if (pos < 0) {
      fprintf(stderr, "Warning: Parameter %s not found in polynomial.\n",
              pv->name);
      continue;
    }
    isl_val *val = isl_val_int_from_si(isl_space_get_ctx(space), pv->value);
    point = isl_point_set_coordinate_val(point, isl_dim_param, pos, val);
  }

  isl_val *eval = isl_pw_qpolynomial_eval(isl_pw_qpolynomial_copy(poly), point);
  int result = isl_val_get_num_si(eval);

  isl_val_free(eval);
  isl_space_free(space);

  return result;
}

/**
 * @brief Compute the number of floating-point operations in an OpenScop
 * representation.
 * @param scop The OpenScop representation of the program.
 * @return The number of floating-point operations.
 */
int compute_flops(param_values_t *params, osl_scop_p scop) {
  int flops = 0;
  unsigned nstmt = 0;

  unsigned total_flops = 0;

  // Traverse the OpenScop representation
  isl_ctx *ctx = isl_ctx_alloc();

  isl_printer *printer = isl_printer_to_file(ctx, stderr);
  printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);

  for (osl_statement_p stmt = scop->statement; stmt != NULL;
       stmt = stmt->next) {
    osl_body_p body = osl_generic_lookup(stmt->extension, OSL_URI_BODY);
    if (body == NULL) {
      continue;
    }
    fprintf(stderr, "# Computing flops for statement %u\n", nstmt);
    fprintf(stderr, "%s\n", body->expression->string[0]);
    flops = compute_flops_in_expr(body->expression->string[0]);
    fprintf(stderr, "STMT FLOPs = %d\n", flops);
    isl_pw_qpolynomial *ehrhart_poly =
        compute_ehrhart_polynomial(ctx, scop, stmt);
    fprintf(stderr, "Ehrhart polynomial: \n");
    printer = isl_printer_print_pw_qpolynomial(printer, ehrhart_poly);
    printer = isl_printer_end_line(printer);

    if (ehrhart_poly == NULL) {
      fprintf(stderr, "Error: Could not compute Ehrhart polynomial.\n");
      continue;
    }
    if (params != NULL) {
      int eval_flops = evaluate_ehrhart_polynomial(ehrhart_poly, params);
      if (eval_flops > 0) {
        total_flops += eval_flops * flops;
        fprintf(stderr, "EVAL FLOPs = %d\n", eval_flops * flops);
      } else {
        fprintf(stderr, "Could not evaluate Ehrhart polynomial.\n");
      }
    }

    fprintf(stderr, "\n");

    isl_pw_qpolynomial_free(ehrhart_poly);
    nstmt++;
  }

  isl_printer_free(printer);

  isl_ctx_free(ctx);

  fprintf(stderr, "\n");
  fprintf(stderr, "Total FLOPs = ");
  fprintf(stdout, "%u\n", total_flops);

  return flops;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input.c> [PARAM_NAME=VALUE]...\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *input_file = argv[1];

  FILE *in = fopen(input_file, "r");
  if (in == NULL) {
    fprintf(stderr, "Error: Could not open file %s\n", input_file);
    return EXIT_FAILURE;
  }

  param_values_t *param_values = NULL;
  /* parse param values*/
  for (int i = 2; i < argc; i++) {
    char *arg = argv[i];
    char *eq = strchr(arg, '=');
    if (eq == NULL) {
      fprintf(stderr, "Warning: Ignoring invalid parameter argument %s\n", arg);
      continue;
    }
    *eq = '\0';
    char *name = arg;
    int value = atoi(eq + 1);
    struct param_values *pv = malloc(sizeof(struct param_values));
    pv->name = name;
    pv->value = value;
    pv->next = param_values;
    param_values = pv;
  }

  clan_options_p clan_opts = clan_options_malloc();

  // Parse the input C program into an OpenScop representation
  osl_scop_p scop = clan_scop_extract(in, clan_opts);
  fclose(in);

  clan_options_free(clan_opts);

  if (scop == NULL) {
    fprintf(stderr, "Error: Failed to parse input file into OpenScop.\n");
    return EXIT_FAILURE;
  }

  fprintf(stderr, "Parsed parameters:\n");
  for (param_values_t *pv = param_values; pv != NULL; pv = pv->next) {
    fprintf(stderr, "Parameter %s = %d\n", pv->name, pv->value);
  }
  fprintf(stderr, "\n");

  // Compute the number of floating-point operations
  compute_flops(param_values, scop);

  // Free the OpenScop representation
  osl_scop_free(scop);

  for (param_values_t *pv = param_values; pv != NULL;) {
    param_values_t *next = pv->next;
    free(pv);
    pv = next;
  }

  return EXIT_SUCCESS;
}