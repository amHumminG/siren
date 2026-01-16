# Siren guidelines

This document summarizes the most important standards used within this project.

## Project Structure

Siren follows a strict separation between the **Public API** and **Internal Logic**.

  * **`siren/include/siren/`**: Public headers *only*.
      * Contains the interface users will see (e.g., `AudioDecoder.h`).
      * No internal implementation details or system headers allowed here.
  * **`siren/src/`**: Implementation files (`.cpp`) and private headers.
      * All logic goes here.
      * **`siren/src/internal/`**: Private tools meant *only* for the library (e.g., `Log.h`).
  * **`application/`**: A sandbox application for testing the library.
  * **`dependencies/`**: All dependencies for the testbed go here
  * **`assets/`**: All assets for the testbed go here

## Coding Standards

The project is using **C++20**.

### Naming Conventions

| Element | Style | Example |
| :--- | :--- | :--- |
| **Namespaces** | `snake_case` | `namespace siren::utils` |
| **Types (Classes/Structs)** | `PascalCase` | `AudioDecoder` |
| **Functions** | `camelCase` | `openFile()` |
| **Variables** | `camelCase` | `bufferSize` |
| **Private Members** | `m_` prefix + `camelCase` | `m_fileHandle` |
| **Files** | `PascalCase` | `AudioDecoder.h` |

### C++ Guidelines

  * **`std::span`**: Use `std::span<float>` instead of raw pointers (`float*`, `size`) for passing buffers.
  * **`[[nodiscard]]`**: Mark all functions that return a `Result` or status code with `[[nodiscard]]`.
  * **`const`**: Mark methods `const` if they do not modify the object state.
  * **`noexcept`**: Mandatory for any function intended to run in the audio callback loop.
  * **`nullptr:`** Always use `nullptr`, never `NULL` or `0`.

## Two Contexts

There are two context (threads) that the code is split between. Know which one you are writing for.
All variables and functions that will be used in the Audio Thread have to be thread safe.

### 1\. The Main Thread (Setup & UI)

Do anything here as long as it does not interfere with the audio thread.

### 2\. The Audio Thread (Real-Time)

**Allowed:** Math, Pointer Arithmetic, Atomic reads/writes.
* **Strictly Forbidden:**
      * Allocating memory (`malloc`, `new`, `std::vector::push_back`).
      * File I/O (`fread`, `std::ofstream`).
      * Throwing Exceptions.
      * Locking Mutexes (blocking).
      * `std::cout` / `printf`.

## The User API Contract

**Internal Safety:** The engine guarantees thread safety between the Simulation (Main Thread) and the Speakers (Audio Thread). Variables shared between these two must be std::atomic or protected by internal mutexes.

**User Responsibility:** The Public API is Single-Threaded. We do not guarantee safety if the user calls siren::Voice methods from multiple different simulation threads simultaneously.

## Internal Audio Format 
Unless otherwise specified, all internal audio processing assumes:
* **Layout:** Interleaved (L R L R).
* **Range:** -1.0 to +1.0 (Hard clipped at the output stage).

## Math & Physics

**Units:**

* **Distance:** Meters (1.0f = 1 meter).
* **Time:** Seconds (1.0f = 1 second).
* **Angles:** Radians (use siren::math constants where possible).
* **Volume:** Linear Amplitude (0.0f to 1.0f). Do not pass Decibels to internal logic unless specified.

## Error Handling

The custom `Result` type is used to handle errors without exceptions in hot paths.

  * **Return Type:** Use `siren::Result<T>` for operations that might fail.
  * **Checking:** Use the explicit bool operator: `if (!result) { ... }`. Alternatively use the `if (!result.isOk()) { ... }` or `if (result.isErr()) { ... }`.
  * **Logging:**
      * In **Public Headers**: Use `siren::utils::logError("msg")`.
      * In **Source Files**: Use `SIREN_LOG_ERROR("msg")` (from `internal/Log.h`).
      * **Prefix:** All logs should be prefixed (handled automatically by the macro/util) to ensure grep-ability.

## Documentation

Use **Doxygen** style comments (`///`) in public headers.

**Recommended Tags (Choose as you see fit):**

  * `@brief`: Short summary.
  * `@param`: Explanation of arguments (include units like seconds/frames).
  * `@return`: What comes back.

<!-- end list -->

```cpp
/// @brief Reads audio frames into the buffer.
/// @param buffer Destination buffer
/// @return Number of frames read.
size_t read(std::span<float> buffer);
```

## Version Control

We use **Conventional Commits** for git messages.

  * **Format:** `type(scope): description`
  * **Types:**
      * `feat`: A new feature
      * `fix`: A bug fix
      * `test`: A test
      * `docs`: Documentation only
      * `style`: Formatting/Whitespace (no code change)
      * `refactor`: Code restructuring without behavior change
      * `chore`: Build scripts, settings, maintenance
