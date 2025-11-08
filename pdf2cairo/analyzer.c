#include "pdfio.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// -- Operator Analysis Structure --
typedef struct 
{
  char name[64]; // Operator name
  int count;     // Number of times it appears
} operator_count_t;

// -- Global variables for analysis  ---
static operator_count_t *op_counts = NULL;
static int num_op_counts = 0;


//
// 'increment_op_count()' - Find and increment the count for an operator.
//
static void
increment_op_count(const char *token)
{
  // First, search if we already have this operator
  for (int i = 0; i < num_op_counts; i++)
  {
    if (!strcmp(op_counts[i].name, token))
    {
      op_counts[i].count++;
      return;
    }
  }

  // If not found, add a new entry
  void *temp = realloc(op_counts, (num_op_counts + 1) * sizeof(operator_count_t));
  if (!temp)
  {
    fprintf(stderr, "ERROR: Out of memory for operator counts!\n");
    return;
  }
  op_counts = temp;

  strncpy(op_counts[num_op_counts].name, token, sizeof(op_counts[0].name) - 1);
  op_counts[num_op_counts].name[sizeof(op_counts[0].name) - 1] = '\0';
  op_counts[num_op_counts].count = 1;
  num_op_counts++;
}


//
// 'analyze_content_stream()' -- Count all operators in the content stream.
//
void
analyze_content_stream(pdfio_stream_t *st)
{
  char token[1024];

  while (pdfioStreamGetToken(st, token, sizeof(token)))
  {
    // We only care about operators, which are not numbers.
    if (!isdigit(token[0]) && token[0] != '.' && token[0] != '-')
    {
      increment_op_count(token);
    }
  }
}
void print_and_free_analysis_summary(void)
{
  
    printf("--- Operator Analysis Summary ---\n");
    for (int i = 0; i < num_op_counts; i++)
      printf(" Fount operator '%s' : '%d' times \n", op_counts[i].name, op_counts[i].count);

    // Free the memory we allocated
    free(op_counts);
    op_counts = NULL;
    num_op_counts = 0;
}

