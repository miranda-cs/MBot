# ⚠ This program is bannable by Vanguard.

# I will not upload releases to this repository because they may be flagged by Riot and you will get banned. You can compile and obfuscate it yourself.

## Attribution

This repository is based on the original KBotExt project by KebsCS:
https://github.com/KebsCS/KBotExt

Original work belongs to the original author. This version contains my own modifications.

## Development

- **Installing required libraries**
  1. Install [GIT for windows](https://git-scm.com/download/win)
  2. Open Windows Command Prompt (**CMD**)
  3. Run `git clone https://github.com/microsoft/vcpkg.git`
  4. `cd vcpkg`
  5. `bootstrap-vcpkg.bat`
  6. `vcpkg integrate install`
  7. `vcpkg install freetype:x64-windows-static`
  8. `vcpkg install cpr:x64-windows-static`
  9. `vcpkg install jsoncpp:x64-windows-static`
- **Building the project**
  1. Clone the repository
  2. Open **KBotExt.sln** in Visual Studio (Recommended Visual Studio 2022)
  3. Set the solution platform to x64 Release
  4. Build the project
  5. Feel free to make a pull request with your changes :-)

