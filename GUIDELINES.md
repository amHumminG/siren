# Siren guidelines

This document summarizes the most important standards used within this project.

## Project Structure

Siren follows a strict separation between the **Public API** and **Internal Logic**.

  * **`siren/include/siren/`**: Public headers *only*.
      * Contains the interface users will see (e.g., `AudioDecoder.h`).
      * No internal implementation details or system headers (`<Windows.h>`) allowed here.
  * **`siren/src/`**: Implementation files (`.cpp`) and private headers.
      * All logic goes here.
      * **`siren/src/internal/`**: Private tools meant *only* for the library (e.g., `Log.h`).
  * **`application/`**: A sandbox application for testing the library.

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

### Modern C++ Guidelines

  * **`std::span`**: Use `std::span<float>` instead of raw pointers (`float*`, `size`) for passing buffers.
  * **`[[nodiscard]]`**: Mark all functions that return a `Result` or status code with `[[nodiscard]]`.
  * **`const`**: Mark methods `const` if they do not modify the object state.
  * **`noexcept`**: Mandatory for any function intended to run in the audio callback loop.

## Two Contexts

Audio programming involves two distinct execution contexts. Know which one you are writing for.

### 1\. The Main Thread (Setup & UI)

  * **Allowed:** Allocating memory (`new`, `std::vector`), File I/O, Exceptions, Locking Mutexes.
  * **Context:** `open()`, `init()`, destructors.

### 2\. The Audio Thread (Real-Time)

  * **Allowed:** Math, Pointer Arithmetic, Atomic reads/writes.
  * **Strictly Forbidden:**
      * Allocating memory (`malloc`, `new`, `std::vector::push_back`).
      * File I/O (`fread`, `std::ofstream`).
      * Throwing Exceptions.
      * Locking Mutexes (blocking).
      * `std::cout` / `printf`.
  * **Context:** `read()`, `process()`, mixing loops.

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

**Required Tags:**

  * `@brief`: Short summary.
  * `@param`: Explanation of arguments (include units like seconds/frames).
  * `@return`: What comes back.
  * **Safety Notes:** explicitly state thread/real-time safety.

<!-- end list -->

```cpp
/// @brief Reads audio frames into the buffer.
/// @return Number of frames read.
/// @note Real-time Safe: YES.
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
