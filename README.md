## WOMM (Works On My Machine)

> **Warning: Unstable Build**
> This project is in active, chaotic development. Expect segfaults, memory leaks, and fluctuating FPS.
> It's not a bug; it's the core gameplay mechanic.

WOMM is a **software architecture simulator**. 

### Building from Source

**Prerequisites:**
*   A C compiler (Clang)
*   Make
*   Bear (for compile database)
*   X11 development libraries (still for Linux)

```bash
# Clone this repository 
git clone https://github.com/kunyukzz/womm.git
cd womm

# Configure the project
bear -- make -f Makefile

# Clean everything
make clean

# Compile everything. 
make

# Run the executable and embrace the chaos
./build/womm
