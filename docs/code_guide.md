# Code Guide

The codebase follows a **component-based design**: each directory is a subsystem, and each subsystem has a clearly defined public surface.

## Terminology

| Term                   | Definition                                                                                                                     |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **Public subsystem**   | A directory containing a header whose name matches the directory (e.g. `type/type.h`). That header is its official public API. |
| **Internal subsystem** | A directory with no matching header. It is private to its parent module.                                                       |
| **Module**             | A directory with no matching header that groups related subsystems. It is a container, not a component.                        |

## Inclusion & Visibility Rules

1. **Public headers only.**
   Code outside a subsystem may include only that subsystem's public header (e.g. `#include "type/type.h"`).

2. **Internal subsystems are sibling-only.**
   A subsystem without a public header is internal to its parent module. It may be used or included only by its **immediate siblings** (other direct children of the same module), and never from outside that module.

3. **avoide cycles.**
   The include graph between subsystems should be acyclic, although this is not strictly necessary. Cycles are legal, but they tend to signal tangled responsibilities, so avoid them where practical. If two headers need each other's full type definitions, break the cycle with a forward declaration or by moving the shared type into a third subsystem, but these are unavoidable for function dependency.

4. **Public headers include only public headers.**
   A public header must not include an internal header, directly or transitively. Exported signatures may mention only exported types.

5. **Same rules for `.c` files.**
   Source files follow the same include rules as headers. Tests may include the internals of the subsystem they test, and nothing else.

## Enforcement

These rules are a **code standard**, not a mechanically enforced contract. C has no namespaces, modules, or access control, so the compiler will not stop a bad `#include`. The rules stand in for the language features C lacks:

| Missing in C            | Replaced by                                                                                       |
| ----------------------- | ------------------------------------------------------------------------------------------------- |
| Namespaces              | One directory per subsystem, with symbols prefixed by the subsystem name (e.g. `type_`, `scope_`) |
| Classes / encapsulation | Public header as the only API; everything else is internal                                        |
| `private` visibility    | `static` for file-local symbols; internal headers for subsystem-local symbols                     |
| Module imports          | Include only public headers, as defined above                                                     |

- **Use the helper.** Run `inclusion.py` to find potential violations. Treat its output as a prompt to review, not a verdict.
- **Follow them by default.** Every new file and include should respect the rules, and reviewers should check for violations.
- **Violations are bugs.** Fix them or document the reason, even though the build will not fail.
- **Exceptions must be explicit.** If a rule has to be broken, leave a comment at the include explaining why.
- **Automated checks are optional.** A script may be added later to report violations, but the rules apply with or without one.

---

# Trident Source Root

## Subsystem Dependencies

- **All Subsystems**: As the top-level driver, `main.c` orchestrates and relies on all primary compiler subsystems to execute the compilation pipeline.

## Subsystems & Modules

- **`core`**: Foundational infrastructure, memory management (arenas), diagnostics, and source buffers.
- **`sema`**: A multipass symantics analyzer with compiler time expression evaluators and compiler hooks with intrensics.
- **`syntax`**: Lexical and syntactic pipeline (lexer, parser, AST structures).
- **`type`**: Type system representations and helper utilities.

---

# Notes

> [!NOTE]
> Any notes regarding the code, architectural decisions, or implementation details excluding inline `TODO` comments — may be documented in the `docs/` on a related `*.md` file.
