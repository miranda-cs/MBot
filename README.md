# MBot

## Warning

This program is bannable by Vanguard.

I will not upload releases to this repository because they may be flagged by Riot. If that happens, using a public release could get you banned. You can compile and obfuscate it yourself.

## Attribution

This repository is based on the original project by KebsCS:
https://github.com/KebsCS/KBotExt

Original work belongs to the original author. This version contains my own modifications.

## Build

1. Install Visual Studio 2022 with the C++ desktop workload.
2. Install and bootstrap vcpkg.
3. Install the required libraries:

```bat
vcpkg install freetype:x64-windows-static
vcpkg install cpr:x64-windows-static
vcpkg install jsoncpp:x64-windows-static
vcpkg integrate install
```

4. Build `MBot.sln` as `Release|x64`, or double-click `Build-Release-x64.ps1`.
