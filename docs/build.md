# Building Trident

> [!TIP]
> to use a different compielr change the `CC` with make. e.g.
>
> ```
> make CC=gcc MODE=release
> ```

> [!NOTE]
> `gperf` needs to be a globally available command.

> [!TIP]
> On windows system, it is recommended to use `WSL` for a smoother experience.

## Linux / Unix Systems

### Dependencies

- `make`
- `clang`
- `gperf`
- `find`
- `python3`

### Build Command

To build the compiler in release mode, run:

```bash
make MODE=release
```
