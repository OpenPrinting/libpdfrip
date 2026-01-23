# libpdfrip

A C-based PDF rendering library that converts PDF pages to PNG images using Cairo graphics. Built for developers who need to understand how PDFs work under the hood.

## What Problem Does This Solve?

PDFs are complicated. Really complicated. If you've ever tried to extract content from a PDF or render it yourself, you know what I'm talking about. Most PDF tools are either black boxes (you can't see what's happening inside) or they're massive commercial libraries.

libpdfrip sits in the middle. It's:
- Small enough to understand and learn from
- Powered by libpdfio (for parsing PDFs) and Cairo (for rendering graphics)
- Written in C, so you can see exactly what's happening at each step
- Designed for learning PDF internals while actually getting useful work done

If you want to understand how PDF rendering works, or if you need a lightweight tool to convert PDFs to images, this project is for you.

## Technologies Used

- **C** - The entire codebase is written in C. No JavaScript, no Node.js, no npm.
- **libpdfio** - Handles PDF parsing and structure navigation
- **Cairo** - 2D graphics library that does the actual rendering to PNG
- **FreeType** - Font rendering support
- **libpng** - PNG image output

## Features

* Render individual PDF pages directly to PNG
* Configurable output resolution (DPI)
* Content stream analysis mode for inspecting PDF operator usage
* Optional verbose logging for detailed diagnostics
* Flexible output naming conventions to support automation and testing

## Project Structure

Here's what lives where:

- **source/** - All the C source code
  - **source/cairo/** - Cairo-specific rendering code (device setup, graphics state, text, paths)
  - **source/pdf/** - PDF interpreter and operator handling
  - **source/tools/pdf2cairo/** - Command-line tool implementation
- **testfiles/** - Sample PDFs and test outputs
  - **testfiles/renderer/** - Input PDF test files
  - **testfiles/renderer-output/** - Generated PNG outputs from tests
- **Makefile** - Build configuration (this is a C project, not Node.js!)
- **test.h** / **testpdf2cairo.c** - Test runner
- **README.md** - You are here

## Building libpdfrip

### ⚠️ Common Beginner Mistake

This is a **C project**. Do not run `npm install`. There is no package.json. There is no Node.js dependency.

If you see `npm install` fail, that's expected - ignore it. You need a C compiler, not Node.

### Dependencies

You need these installed before building:

* C compiler (gcc or clang)
* make
* pkg-config
* libpdfio (development headers)
* cairo (development headers)
* freetype2 (development headers)
* libpng (development headers)

### On Debian/Ubuntu

```bash
sudo apt-get install build-essential pkg-config libpdfio-dev libcairo2-dev libfreetype6-dev libpng-dev
```

### Building

```bash
git clone https://github.com/OpenPrinting/libpdfrip.git
cd libpdfrip
make
```

This produces:

* `pdf2cairo/pdf2cairo_main` – primary rendering and analysis tool
* `testpdf2cairo` – test runner

If you get errors about missing headers, you probably forgot to install the `-dev` packages listed above.

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
| `-p`        | `<pagenum>`    | Page number to process (default: 1).                               |
| `-r`        | `<dpi>`        | Output resolution in DPI (default: 72).                            |
| `-t`        |                | Generate a temporary output filename (requires `-d`).              |
| `-d`        | `<directory>`  | Output directory when using `-t`.                                  |
| `-T`        |                | Generate a temporary filename inside `testfiles/renderer-output/`. |
| `-v`        |                | Enable verbose diagnostic output.                                  |

### Examples

Render page 1 to PNG:

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

---

## Understanding PDFs (For Beginners)

If you're new to PDF internals, here's what you need to know to work on this project.

### PDFs Are Programs, Not Documents

This is the biggest mindshift: a PDF isn't really a "document" in the way a text file is. It's more like a program that tells a renderer what to draw.

When you open a PDF, the viewer executes drawing commands like:
- "Move to coordinate (100, 200)"
- "Draw a line to (150, 300)"
- "Fill this path with red"
- "Show the text 'Hello World' at the current position"

### Pages

A PDF contains one or more pages. Each page has:

- **Resources** - Fonts, images, and reusable graphics that the page needs
- **Content Stream** - A sequence of drawing commands (the "program" that draws the page)
- **MediaBox** - The physical size of the page (like 8.5" × 11")

### Content Streams

The content stream is where the action happens. It's a list of PDF operators like:

```
10 20 m          % Move to (10, 20)
100 20 l         % Line to (100, 20)
100 100 l        % Line to (100, 100)
10 100 l         % Line to (10, 100)
h                % Close path
S                % Stroke (draw the outline)
```

Our interpreter reads these commands one by one and tells Cairo what to draw.

### XObjects (External Objects)

XObjects are reusable content. Instead of repeating the same drawing commands over and over, you define an XObject once and reference it multiple times.

There are two main types:
- **Image XObjects** - Embedded images (JPEG, PNG, etc.)
- **Form XObjects** - Reusable vector graphics and text (not interactive forms!)

Think of Form XObjects like functions in C - you define them once and call them whenever needed.

---

## Form XObjects - A Deep Dive

Form XObjects are everywhere in real PDFs. If you're going to work on this project, you need to understand them.

### What Is a Form XObject?

A Form XObject is a **self-contained chunk of PDF content** that you can reuse. It's like copying a bunch of drawing commands into a function, then calling that function whenever you want to draw that content.

**Real-world example**: A company logo that appears on every page. Instead of including the logo's drawing commands 50 times (once per page), you define it as a Form XObject and reference it 50 times. The PDF is smaller, and rendering can be faster (because the renderer can cache the result).

### Anatomy of a Form XObject

A Form XObject is a PDF stream with these key entries:

```
<<
  /Type /XObject
  /Subtype /Form        % "I'm a Form, not an Image"
  /BBox [0 0 100 50]    % My coordinate space
  /Matrix [1 0 0 1 0 0] % How to transform me
  /Resources << ... >>  % Fonts, images I need
>>
stream
% Drawing commands go here (just like a page content stream)
1 0 0 rg               % Set color to red
0 0 100 50 re          % Rectangle from (0,0) to (100,50)
f                      % Fill it
endstream
```

### Key Dictionary Entries

#### /Subtype /Form

This says "I'm a Form XObject, not an Image XObject." When you see `/Type /XObject`, you need to check the Subtype to know what you're dealing with.

#### /BBox (Bounding Box)

`/BBox [x_min y_min x_max y_max]`

This defines the Form's **own coordinate system**. Everything drawn inside the Form uses these coordinates.

Example: `/BBox [0 0 200 100]` means the Form has a coordinate space from (0, 0) to (200, 100).

#### /Matrix (Transformation Matrix)

`/Matrix [a b c d e f]`

This is a 6-number transformation matrix (like you'd use in linear algebra or OpenGL). It transforms the Form's coordinate space when you place it on a page.

Default: `[1 0 0 1 0 0]` (identity matrix - no transformation)

The matrix handles:
- **Scaling** - Make the Form bigger or smaller
- **Rotation** - Rotate the Form
- **Translation** - Move the Form to a different position
- **Skewing** - Distort the Form (rarely used)

You don't need to understand matrix math to work on this project, but if you're curious, it's a standard 2D affine transformation matrix.

#### /Resources

Just like a page, a Form XObject can have its own Resources dictionary:

```
/Resources <<
  /Font << /F1 10 0 R >>
  /XObject << /Image1 20 0 R >>
>>
```

This tells the Form what fonts, images, or even other Form XObjects it needs.

### The Do Operator - Invoking a Form

To use a Form XObject, you reference it in your Resources and then use the `Do` operator:

```
% In the page's Resources:
/Resources <<
  /XObject << /Logo 42 0 R >>  % "Logo" points to a Form XObject
>>

% In the page's content stream:
q                   % Save graphics state
1 0 0 1 100 200 cm  % Move to position (100, 200)
/Logo Do            % Execute the Form XObject named "Logo"
Q                   % Restore graphics state
```

When the renderer encounters `Do`:

1. **Save the current state** (like pushing a stack frame)
2. **Apply the Form's /Matrix transformation**
3. **Set up the Form's resources** (fonts, images, etc.)
4. **Execute the Form's content stream** (process all its drawing commands)
5. **Restore the previous state** (pop the stack)

It's almost exactly like calling a function in C, except the "function body" is a stream of PDF operators.

### Why Form XObjects Matter

You'll encounter Form XObjects constantly:

- **Repeated content** - Headers, footers, logos, watermarks
- **File size optimization** - Complex graphics stored once, referenced many times
- **PDF forms** - Yes, confusingly, interactive PDF form fields often use Form XObjects to draw buttons, checkboxes, etc.
- **Layers and structure** - Some PDFs use Form XObjects to organize content logically

If your PDF renderer doesn't handle Form XObjects, you'll fail on the vast majority of real-world PDFs.

### In the libpdfrip Code

When you're working on the interpreter, you'll see code that:

1. Detects the `Do` operator
2. Looks up the XObject name in the current Resources
3. Checks if it's a Form (as opposed to an Image)
4. Saves the graphics state
5. Applies the Form's Matrix
6. Recursively processes the Form's content stream
7. Restores the graphics state

This recursive processing is why PDF rendering can be tricky - Forms can contain Forms can contain Forms...

---

## Contributing

We welcome contributions! This project is a great way to learn about PDF internals and C graphics programming.

### Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/libpdfrip.git
   cd libpdfrip
   ```
3. **Install dependencies** (see the Building section above)
4. **Build the project**:
   ```bash
   make
   ```
5. **Run the tests** to make sure everything works:
   ```bash
   make test
   ```

### Making Changes

1. **Create a branch** for your work:
   ```bash
   git checkout -b fix-text-rendering
   ```
2. **Make your changes** - Start small! Fix one bug or add one small feature.
3. **Test your changes**:
   ```bash
   make clean
   make
   make test
   ```
4. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Fix text positioning in rotated content streams"
   ```
5. **Push to your fork**:
   ```bash
   git push origin fix-text-rendering
   ```
6. **Open a pull request** on the main repository

### What Makes a Good Contribution?

- **Small and focused** - Fix one thing at a time
- **Well-tested** - Make sure existing tests pass and add new tests if needed
- **Explained** - Your commit message should explain what you changed and why
- **Follows existing code style** - Look at the surrounding code and match it

Don't worry about making your first contribution perfect. We'd rather see a small, imperfect fix than wait for a massive perfect rewrite.

### Areas Where We Need Help

- **Bug fixes** - Especially rendering issues with specific PDFs
- **Test coverage** - More test PDFs and test cases
- **Documentation** - Explaining PDF operators and rendering concepts
- **Performance** - Optimizing hot paths in the renderer
- **New operators** - Implementing PDF operators we don't support yet

### Questions?

If you're stuck or not sure how to approach something:

- Open an issue on GitHub and ask
- Look at recent pull requests to see how others have contributed
- Check the [PDF Reference](https://opensource.adobe.com/dc-acrobat-sdk-docs/pdfstandards/PDF32000_2008.pdf) if you're confused about PDF behavior

We're here to help. Everyone starts somewhere, and PDF is genuinely complicated.

---

## Contributing

Contributions are welcomed. All pull requests must:

* Pass the existing test suite (`make test`).
* Follow the current code structure and formatting conventions.

