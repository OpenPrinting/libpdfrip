//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "pdfops-private.h"

//
// 'getPDFdata()' - get all metadata of PDF file
//

pdfrip_doc_t*				  // O - Data structure with all values of PDF
getPDFdata(pdfio_file_t *pdf)		// I - PDF file in pdfio_file_t
{
  if(pdf == NULL)
  {
    fprintf(stderr, "ERROR: No PDF file\n");
    return NULL;
  }

  pdfrip_doc_t *PDF_data = (pdfrip_doc_t*)malloc(sizeof(pdfrip_doc_t));
  
  PDF_data->pdf 	  = pdf;
  PDF_data->version 	  = pdfioFileGetVersion(pdf);

  PDF_data->num_pages 	  = pdfioFileGetNumPages(pdf);
  if(PDF_data->num_pages == 0)
  {
    fprintf(stderr, "ERROR: PDF file has no pages\n");
    return NULL;
  }

  PDF_data->num_objects   = pdfioFileGetNumObjs(pdf);
  if(PDF_data->num_objects == 0)
  {
    fprintf(stderr, "ERROR: PDF file has no objects\n");
    return NULL;
  }

  PDF_data->catalog_dict  = pdfioFileGetCatalog(pdf);
  if(!PDF_data->num_objects)
  {
    fprintf(stderr, "ERROR: PDF file has no catalog dictionary\n");
    return NULL;
  }

  return PDF_data;
}

//
// 'openPDFfile()' - get all metadata of PDF file from only the path
//

pdfrip_doc_t*				  // O - Data structure with all values of PDF
openPDFfile(char* filename)		// I - PDF filename
{
  pdfio_file_t *pdf = pdfioFileOpen(filename, NULL, NULL, NULL, NULL);
  if(!pdf)
  {
    fprintf(stderr, "ERROR: No PDF file\n");
    return NULL;
  }

  pdfrip_doc_t *PDF_data = getPDFdata(pdf);

  if(!PDF_data)
  {
    fprintf(stderr, "ERROR: No PDF file data\n");
    return NULL;
  }
 
  return PDF_data;
}

//
// 'freePDFdoc()' - free the pdfrip_doc_t data structure 
//

void 					  // O - Void output
freePDFdoc(pdfrip_doc_t *PDF_data)	// I - ripPDF doc
{
  pdfioFileClose(PDF_data->pdf);
  free(PDF_data);
}

//
// 'getPageData()' - get Page data from PDF
//

pdfrip_page_t* 				  // O - Page data structure
getPageData(pdfio_file_t *pdf,		// I - PDF file
	    size_t page_number)		// I - Page number ( 0-based index)
{
  pdfrip_page_t *page = (pdfrip_page_t*)malloc(sizeof(pdfrip_page_t));

  page->object = pdfioFileGetPage(pdf, page_number);
  page->object_dict = pdfioObjGetDict(page->object);
  page->resources_dict = pdfioDictGetDict(page->object_dict, "Resources");

  if(!pdfioDictGetRect(page->object_dict, "TrimBox", &page->mediaBox) && 
      !pdfioDictGetRect(page->object_dict, "BleedBox", &page->mediaBox))
  {
    if(!pdfioDictGetRect(page->object_dict, "CropBox", &page->mediaBox))
    {
      pdfioDictGetRect(page->object_dict, "MediaBox", &page->mediaBox);
    } 
  }

  page->object_number = pdfioObjGetNumber(page->object);
  page->gen_number = pdfioObjGetGeneration(page->object);
  page->num_streams = pdfioPageGetNumStreams(page->object);

  return page;
}

//
// 'freePageData()' - free Page data structure
//

void 
freePageData(pdfrip_page_t *page)
{
  free(page); 
}
