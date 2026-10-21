# glad (generated OpenGL loader)

These files are **generated** by [glad2](https://github.com/Dav1dde/glad) and committed so the
build needs no Python. Do not edit them by hand.

- Generator: `glad2==2.0.8`
- API: `gl:core=4.6` (covers 4.3 compute shaders for later; the app still requests a 3.3 context
  and only uses what the driver actually provides)
- Extensions: `GL_KHR_debug`
- Licence: the generated code is `(WTFPL OR CC0-1.0) AND Apache-2.0` (see the header comments)

## Regenerating

```powershell
python -m venv $env:TEMP\gladvenv
& $env:TEMP\gladvenv\Scripts\python.exe -m pip install "glad2==2.0.8"
& $env:TEMP\gladvenv\Scripts\glad.exe --api "gl:core=4.6" --extensions "GL_KHR_debug" --out-path third_party/glad c
```

Run it from the repository root. It overwrites `include/` and `src/`.
