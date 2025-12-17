//                                                                                         
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>                              
// Copyright 2025 Uddhav Phatak <uddhavphatak@gmail.com>                                   
//                                                                                         
// Licensed under Apache License v2.0.  See the file "LICENSE" for more                    
// information.                                                                            
//             

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdfops-private.h"
#include "interpreter.h"
#include "cairo_device.h"

//
// 'main()' - Main entry for the pdf2cairo
//

int            		  // O - Exit status
main(int argc, 		// I - Number of command-line args
     char **argv) 	// I - Command-line arguments
{
  // Command-line options
  char 			*filename;		// input PDF filename	
  char 			*output_filename = NULL;
  pdfrip_doc_t 		*PDF_doc;		// PDF meta data structure
  size_t 		i;			// iterator
  int 			dpi = 72;
 
  filename = argv[1];
  PDF_doc = openPDFfile(filename);	

  for(i=0; i<PDF_doc->num_pages ; i++)
  {
    pdfrip_page_t *page = getPageData(PDF_doc->pdf, i); 
    
    p2c_device_t *dev = device_create(page->mediaBox, dpi);
    if (dev)
    {
      device_set_page(dev, page->object);
      
      process_content_stream(dev, page);
      device_save_to_png(dev, output_filename);
      device_destroy(dev);
    }
    freePageData(page);
  }

  fprintf(stderr, "%s\n", PDF_doc->version);
  freePDFdoc(PDF_doc);

  return 0;
}
