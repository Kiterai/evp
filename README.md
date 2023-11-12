# EVP (Efficient VcPkg)

An utility to use [vcpkg](https://github.com/microsoft/vcpkg) easily.

## Basic Commands

### `init`

Initializes a project.

```
$ evp init my-project
```

Files path structure:
```
.gitignore
evp.yml
src/
  main.cpp
```

### `build`

Builds the project in current directory.

```
$ evp build
```

The command makes `build` directory and creates binaries there.

### `run`

Runs a executable in the project in current directory.

```
$ evp run
```
```
$ evp run -t my-app
```
