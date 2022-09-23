## How to compile using WDK 7.1.0

1. To compile this sample using WDK 7.1.0, first you need to configure a build environment:

```
C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\ chk x64 WIN7 no_oacr
```

The command above configures a `checked` build environment (with debugging symbols). To configure optimized build environment, use `free` instead.

2. Build the driver:

```
build
```

