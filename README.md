# Screen Renderer

A C++ OpenGL application for simulating binary screens with configurable dimensions and colors.

## Features

- **Binary screen simulation**: Each pixel can be either on (black) or off (background color)
- **Configurable dimensions**: Default 128x64, but can be changed to any size
- **Configurable colors**: Default blue background (RGB: 0.0, 0.5, 1.0) with black pixels (RGB: 0.0, 0.0, 0.0)
- **OpenGL-based rendering**: Efficient GPU-accelerated rendering using modern OpenGL
- **Test pattern**: Includes a demo pattern showing borders,diagonals, and a checkerboard
- **Bitmap rendering**: Draw pre-made bitmap icons (smiley, heart, arrows, checkmark, cross)
- **Text rendering**: Display text using a built-in 5x7 pixel bitmap font
- **Multiple demos**: Example programs showcasing bitmap and text rendering capabilities

## Building

### Dependencies

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenGL
- GLFW3
- GLEW

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt install -y libglfw3-dev libglew-dev
```

### Build Instructions

```bash
chmod +x build.sh
./build.sh
```

Or manually:

```bash
mkdir -p build
cd build
cmake ..
make
```

## Running

After building, run the application:

```bash
./build/screen-renderer
```

This will open a window displaying a 128x64 binary screen with a test pattern.

### Run Example Programs

**Bitmap Demo** - Shows various bitmap icons rendered on the screen:
```bash
./build/bitmap-demo
```

**Text Demo** - Demonstrates text rendering with the 5x7 pixel font:
```bash
./build/text-demo
```

**Interactive Demo** - Full-featured GUI testing tool with ImGui:
```bash
./build/interactive-demo
```

Features:
- Text input field with position and spacing controls
- Bitmap placement with dropdown selector for all icon types
- Interactive pixel drawing (click and drag on screen)
- Screen controls: clear, fill, invert, and draw border
- Color pickers for background and pixel colors
- Real-time pixel count statistics

## Customization

### Changing Screen Size

Edit `src/main.cpp` and modify the screen construction:

```cpp
// Create a 256x128 screen instead of 128x64
screen_t screen(256, 128);
```

### Changing Colors

Edit `src/main.cpp` and modify the renderer construction:

```cpp
// Create renderer with red background and white pixels
screen_renderer_t renderer(1.0f, 0.0f, 0.0f,  // Red background
                           1.0f, 1.0f, 1.0f); // White pixels
```

### Drawing Custom Patterns

The `draw_test_pattern` function in `src/main.cpp` demonstrates how to manipulate pixels. You can modify this function or create your own:

```cpp
// Clear the screen (all pixels off)
screen.clear();

// Set individual pixels on
screen.set_pixel(10, 20, true);

// Fill the entire screen
screen.fill();
```

### Drawing Bitmaps

The library includes several pre-made bitmap icons:

```cpp
#include "bitmap.hpp"

// Create bitmap icons
bitmap_t smiley = bitmap_t::create_smiley();
bitmap_t heart = bitmap_t::create_heart();
bitmap_t arrow_up = bitmap_t::create_arrow_up();
bitmap_t checkmark = bitmap_t::create_checkmark();

// Draw bitmaps at specific coordinates
screen.draw_bitmap(smiley, 10, 10);
screen.draw_bitmap(heart, 30, 10);
```

Available bitmap factory methods:
- `create_smiley()` - 16x16 smiley face
- `create_heart()` - 16x16 heart shape
- `create_arrow_up()` - 12x12 up arrow
- `create_arrow_down()` - 12x12 down arrow
- `create_arrow_left()` - 12x12 left arrow
- `create_arrow_right()` - 12x12 right arrow
- `create_checkmark()` - 12x12 checkmark
- `create_cross()` - 12x12 X mark

You can also create custom bitmaps:
```cpp
// Create a custom 8x8 bitmap
bitmap_t custom(8, 8);
custom.set_pixel(0, 0, true);
custom.set_pixel(7, 7, true);
screen.draw_bitmap(custom, 50, 20);
```

### Rendering Text

The library includes a built-in 5x7 pixel bitmap font supporting ASCII characters (32-126):

```cpp
#include "font.hpp"

// Create font
font_t font;

// Draw text at specific coordinates
screen.draw_text(font, "HELLO!", 10, 10);
screen.draw_text(font, "Score: 123", 10, 20);

// Adjust character spacing (default is 1 pixel)
screen.draw_text(font, "SPACED", 10, 30, 2);
```

The font supports:
- Uppercase letters (A-Z)
- Lowercase letters (a-z)
- Numbers (0-9)
- Special characters and symbols

## Project Structure

```
screen-renderer/
├── include/
│   ├── screen.hpp           # Screen buffer management
│   ├── shader.hpp           # OpenGL shader utilities
│   ├── screen_renderer.hpp  # OpenGL rendering
│   ├── bitmap.hpp           # Bitmap image support
│   └── font.hpp             # Bitmap font support
├── src/
│   ├── main.cpp             # Application entry point
│   ├── screen.cpp           # Screen implementation
│   ├── shader.cpp           # Shader implementation
│   ├── screen_renderer.cpp  # Renderer implementation
│   ├── bitmap.cpp           # Bitmap implementation
│   └── font.cpp             # Font implementation with 5x7 font data
├── examples/
│   ├── bitmap_demo.cpp      # Bitmap rendering demo
│   ├── text_demo.cpp        # Text rendering demo
│   └── interactive_demo.cpp # Interactive ImGui testing tool
├── CMakeLists.txt           # Build configuration
├── build.sh                 # Build script
└── README.md                # This file
```

## Code Style

This project follows the conventions outlined in `style_guide.md`:
- Snake case naming with `_t` suffix for classes
- Member variables prefixed with `m_`
- Opening braces on new lines
- 2-space indentation
- C++17 features including `auto` return types

## License

This project is open source and available for educational and commercial use.
