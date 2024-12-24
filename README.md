# Raytracer

A GPU-accelerated raytracing application built with OpenGL and GLSL.

![Placeholder Image](images/rt1_final.gif)

## Features
- **Physically-Based Rendering:** Implements materials such as diffuse, metallic, and glass.
- **Anti-Aliasing:** Produces smooth images with multi-sampling.
- **Dynamic Lighting:** Realistic lighting and shadow effects.
- **Temporal Accumulation:** Improves image quality over successive frames.

## Dependencies
This project uses the following libraries:
- GLFW: For window management.
- GLAD: For OpenGL function loading.
- stb_image: For image loading.
- stb_truetype: For TrueType font rendering.
- glm: For mathematical operations.
- Dear ImGui: For creating user interfaces.

## Getting Started

### Prerequisites
- A C++ compiler with support for C++17 or higher.
- CMake (minimum version 3.16).
- OpenGL 4.3 or later.

### Setup and Build
```bash
git clone https://github.com/yourusername/raytracer.git
cd raytracer
mkdir bin
cd bin
cmake ..
cmake --build .
./mygame

