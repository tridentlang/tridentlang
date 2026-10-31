# SUBSYSTEM : Core

The `core` subsystem provides fundamental, cross-cutting infrastructure and shared utilities used across the entire compiler, focusing on memory management, diagnostics, and source file handling.

## Files and SubSystems

### Files

- **`def.h`**: Project-wide common type definitions, primitive bounds, macros, and compiler configuration constants.

### Subsystems

- **`mem`**: General memory wrapping utilities, safety checks, and allocation tracking helpers.
- **`diags`**: Comprehensive diagnostic engine responsible for formatting and reporting errors, warnings, and notes with precise source location markers.
- **`source`**: Source file loading, memory-mapped or buffered file management, and line-column coordinate mapping.

## Dependencies

This is the foundational root subsystem of the compiler and depends only on standard C libraries. All other subsystems depend upon `core`.

# Notes

> [!NOTE]
>
> **TODO**: Use `mmap` for memory-allocation in arena and impliment `xfree` for freeing `xmallo`c memory. the usage in core doesnt really need `xrelloc` to be reimplimented.

> [!NOTE]
> **TODO**: Implement dedicated file-system (`fs`) subsystem.
>
> - **`fs-subsystem`**
>   - **File Discovery & Traversal**
>     - Implement recursive directory walking for module imports.
>     - Support source file path resolution relative to workspace roots.
>   - **File Management Operations**
>     - Implement safe file reading buffers for the lexer/parser input stream.
>     - Handle file writing for compilation artifacts.
>   - **Cache & Temp Folder Management**
>     - Setup localized `.tr-cache` or temporary directories for intermediate build states.
>     - Implement stale artifact cleanup routines.

> [!NOTE]
> **TODO**: Implement coustom flag-parser (`flags`) subsystem.
