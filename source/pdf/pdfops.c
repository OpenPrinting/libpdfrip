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
getPageData(pdfrip_doc_t *pdf_doc,	// I - PDF file
	    size_t page_number)		// I - Page number ( 0-based index)
{
  pdfrip_page_t *page = (pdfrip_page_t*)malloc(sizeof(pdfrip_page_t));

  page->parent_doc = pdf_doc;
  page->object = pdfioFileGetPage(pdf_doc->pdf, page_number);
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
// 'pdf_dict_get_dict()' - Get a dictionary value, following an indirect reference.
//
// pdfioDictGetDict() only returns *direct* dictionaries, but values such as
// /FontDescriptor are almost always indirect ("7 0 R").
//

pdfio_dict_t *				  // O - Dictionary or NULL
pdf_dict_get_dict(pdfio_dict_t *dict,	// I - Dictionary
                  const char   *key)	// I - Key
{
  pdfio_dict_t *value = pdfioDictGetDict(dict, key);

  return value ? value : pdfioObjGetDict(pdfioDictGetObj(dict, key));
}

//
// 'pdf_dict_get_array()' - Get an array value, following an indirect reference.
//

pdfio_array_t *				  // O - Array or NULL
pdf_dict_get_array(pdfio_dict_t *dict,	// I - Dictionary
                   const char   *key)	// I - Key
{
  pdfio_array_t *value = pdfioDictGetArray(dict, key);

  return value ? value : pdfioObjGetArray(pdfioDictGetObj(dict, key));
}

//
// 'pdf_read_stream()' - Read a whole stream object into memory.
//
// With decode false the data is returned as stored, e.g. the JPEG data of a
// DCTDecode image (PDFio itself can only decode FlateDecode).
//

unsigned char *				  // O - Data (free with free()) or NULL
pdf_read_stream(pdfio_obj_t *obj,	// I - Stream object
                bool        decode,	// I - Decode the stream filter?
                size_t      *size)	// O - Number of bytes
{
  pdfio_stream_t *st;
  size_t	capacity = 64 * 1024, total = 0;
  unsigned char	*buffer, *temp;
  ssize_t	bytes;

  *size = 0;

  if (!obj || (st = pdfioObjOpenStream(obj, decode)) == NULL)
    return (NULL);

  if ((buffer = malloc(capacity)) == NULL)
  {
    pdfioStreamClose(st);
    return (NULL);
  }

  while ((bytes = pdfioStreamRead(st, buffer + total, capacity - total)) > 0)
  {
    total += (size_t)bytes;

    if (total == capacity)
    {
      if ((temp = realloc(buffer, capacity * 2)) == NULL)
      {
        free(buffer);
        pdfioStreamClose(st);
        return (NULL);
      }

      buffer = temp;
      capacity *= 2;
    }
  }

  pdfioStreamClose(st);

  if (total == 0)
  {
    free(buffer);
    return (NULL);
  }

  *size = total;
  return (buffer);
}

//
// 'freePageData()' - free Page data structure
//

void 
freePageData(pdfrip_page_t *page)
{
  free(page); 
}
