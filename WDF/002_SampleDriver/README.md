## How to compile and use this sample using WDK 7.1.0

1. To compile this sample using WDK 7.1.0, first you need to configure a build environment:

```
C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\ chk x64 WIN7 no_oacr
```

The command above configures a `checked` build environment (with debugging symbols). To configure optimized build environment, use `free` instead.

2. Build the driver:

```
build
```

3. Generate TMF files:

```
tracepdb -f SampleDriver.pdb
```

4. Run `TraceView.exe` as Administrator and Create a New Tracing Session.
5. Choose the `SampleDriver.ctl` as the Control GUID.
6. Choose the generated `.tmf` file.
7. Install the Driver using `SampleDriver.inf`.
8. Check the generated trace logs in TraceView.
