//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef PDFOPS_PRIVATE_H
#define PDFOPS_PRIVATE_H

#include <stdio.h>
#include <stdlib.h>
#include <pdfio.h>

typedef struct pdfrip_doc_s
{
  pdfio_file_t 	  *pdf;			// PDF file	
  const char	  *version;		// Version of the PDF file
  size_t 	  num_pages,		// Number of Pages in PDF file
		  num_objects;		// Number of Objects in PDF file
  pdfio_dict_t 	  *catalog_dict; 	// Catalog Dictionary of PDF file
} pdfrip_doc_t;

pdfrip_doc_t* getPDFdata(pdfio_file_t *pdf); 		// get all metadata of PDF file
pdfrip_doc_t* openPDFfile(char* filename);		// open PDF file
void 	      freePDFdoc(pdfrip_doc_t *PDF_data);	// free the data structure

typedef struct pdfrip_page_s
{
  pdfio_obj_t		*object;		// Page Object
  pdfio_obj_t		*resource_object;	// Page Resource Object
  pdfio_dict_t		*object_dict,		// Page Dictionary
			*resources_dict;	// Page Resources Dictionary 	
  pdfio_rect_t 		mediaBox;		// Page MediaBox 
  size_t 		object_number, 		// Object Number
  	 		num_streams;		// Number of Object Streams
  unsigned short 	gen_number;		// Object Generation Number
} pdfrip_page_t;

pdfrip_page_t* getPageData(pdfio_file_t *pdf, size_t page_number); // get Page Data from PDF
void freePageData(pdfrip_page_t *page);
#endif
