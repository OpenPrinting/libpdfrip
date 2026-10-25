# libpdfrip

libpdfrip is a high-performance PDF rendering and analysis tool built on top of **libpdfio** and the **Cairo 2D** graphics library. It provides efficient page rendering to PNG format and includes a content stream inspection utility for debugging and development.

## Features

* Render individual PDF pages directly to PNG.
* Configurable output resolution (DPI).
* Content stream analysis mode for inspecting PDF operator usage.
* Optional verbose logging for detailed diagnostics.
* Flexible output naming conventions to support automation and testing.

## Dependencies

The following libraries and tools must be installed:

* C compiler (gcc or clang)
* make
* pkg-config
* libpdfio (development headers)
* cairo (development headers)
* FreeType (development headers)
* fontconfig (development headers)
* libjpeg (development headers)
* libpng (development headers)

### Debian/Ubuntu Installation

```
sudo apt-get install build-essential pkg-config libpdfio-dev libcairo2-dev libfreetype-dev libfontconfig-dev libjpeg-dev libpng-dev
```

## Building

```
git clone https://github.com/OpenPrinting/libpdfrip.git
cd libpdfrip
make
```

This produces:

* `pdf2cairo/pdf2cairo_main` – primary rendering and analysis tool
* `testpdf2cairo` – test runner

## Usage

```
./pdf2cairo/pdf2cairo_main [options] input.pdf
```

### Options

| Flag        | Argument       | Description                                                        |
| ----------- | -------------- | ------------------------------------------------------------------ |
| `--analyze` |                | Analyze PDF content streams instead of rendering output.           |
| `--help`    |                | Display usage information.                                         |
| `-o`        | `<output.png>` | Output PNG filename when rendering.                                |
| `-p`        | `<pagenum>`    | Render only this page (default: every page).                       |
| `-r`        | `<dpi>`        | Output resolution in DPI (default: 72).                            |
| `-t`        |                | Generate a temporary output filename (requires `-d`).              |
| `-d`        | `<directory>`  | Output directory when using `-t`.                                  |
| `-T`        |                | Generate a temporary filename inside `testfiles/renderer-output/`. |
| `-v`        |                | Enable verbose diagnostic output.                                  |

Every page is rendered to its own PNG. For a multi-page PDF the page number
is added to the output filename: `-o output.png` writes `output-1.png`,
`output-2.png`, and so on (zero-padded, e.g. `output-01.png`, when there are
10 or more pages). A single-page PDF, or a single page selected with `-p`, is
written to the output filename unchanged.

### Examples

Render every page to PNG (`output.png`, or `output-1.png`, `output-2.png`, ...):

```
./pdf2cairo/pdf2cairo_main -o output.png document.pdf
```

Render page 5 at 300 DPI:

```
./pdf2cairo/pdf2cairo_main -p 5 -r 300 -o high-res.png document.pdf
```

Analyze page 2 content stream:

```
./pdf2cairo/pdf2cairo_main --analyze -p 2 document.pdf
```

## Testing

```
make test
```

Test output images are written to:

```
testfiles/renderer-output/
```

## Contributing

Contributions are welcomed. All pull requests must:

* Pass the existing test suite (`make test`).
* Follow the current code structure and formatting conventions.

