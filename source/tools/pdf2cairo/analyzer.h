#ifndef ANALYZER_H
#define ANALYZER_H

#include "pdfio.h"

/**
 * @brief Reads a PDF content stream and counts the operators within it.
 *
 * @param[in] st The PDF stream to analyze.
 */
void analyze_content_stream(pdfio_stream_t *st);

/**
 * @breif Prints the summary of operator counts to the console and frees memory
 */
void print_and_free_analysis_summary(void);

#endif
