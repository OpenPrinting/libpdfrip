# libpdfrip

libpdfrip is a C library for rendering PDF pages to PNG images using the Cairo graphics library and PDFio for PDF parsing.

## Purpose

libpdfrip provides PDF page rendering functionality for applications that need to convert PDF documents to raster images. The library uses PDFio to parse PDF structure and Cairo to render vector graphics and text to PNG output.

## Requirements

The following tools and libraries are required to build libpdfrip:

* C compiler (gcc or clang)
* make
* pkg-config
* PDFio
* Cairo
* FreeType2
* libpng

### Installing tools on Debian/Ubuntu

Install the required packages with:

```bash
sudo apt-get install build-essential pkg-config libpdfio-dev libcairo2-dev libfreetype6-dev libpng-dev
```

## Building libpdfrip

To build libpdfrip from source:

```bash
git clone https://github.com/OpenPrinting/libpdfrip.git
cd libpdfrip
make
```

The build produces the following executables:

* `pdf2cairo/pdf2cairo_main` - PDF rendering and analysis tool
* `testpdf2cairo` - test runner

## Documentation

Detailed contributor documentation is available in the `docs/` directory, including background material on PDF internals and Form XObjects.

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on submitting pull requests and reporting issues.

## License

See [LICENSE](LICENSE) and [NOTICE](NOTICE) for license information.
