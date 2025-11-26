# ACAD

ACAD is a Windows application for designing built-up model aircraft wings.
It was orginally written as an exercise as I needed to transition from C to C++ for work.
It has proved so useful that I have maintained and developed it.

It has the following features to enable the creation of complex wing designs:

* Planform definition
* Airfoil definition
* Rib and spacer placements and definitions
* Geodetic rib placements
* Rib parameters such as washout, trailing edge thickness and manual slot keepouts
* Sheet spar and jig definitions
* Strip, box and H spar definitions
* Automated lightening hole creation
* Tube and bar section elements
* Wing sheeting jig definitions
* LE radius templates
* Preview tabs for the wing plan and the generated wing parts

For more information, see the User Guide in the docs directory.

ACAD is licensed with GPLv3; see [license.txt](https://github.com/arniemansell/acad/blob/main/docs/license.txt).

# Installation

If you just want to use ACAD, an installer is provided in releases for Windows 11 64-bit platforms.

If you want to run on a different platform you will need to build ACAD yourself.

# Building ACAD from Source

Instructions in this section are for building from source for the Windows 11 64-bit platform.

The build environment is CMAKE; these instructions use MSVC2022 as the compiler compiler.

The GUI is implemented with Qt.  Instructions are included below to build Qt from source.  If you want to change any of the Qt DLLs, simply replace the .dll file in the executable directory.

## Dependencies

  - Install MSVC 2022 from the microsoft app store, with (at least) Desktop Development in C++ support.
  - Install cmake [from here](https://cmake.org/download/), making sure to ckeck the "Add to path" option.
  - Download ninja [from here](https://github.com/ninja-build/ninja/releases) and copy the binary to same location as the cmake binary.
  - Install Python 3 from the Microsoft Store.
  - Install ActiveState Perl [from here](https://platform.activestate.com).

## Get the ACAD Source Code

Start by cloning this repository.

## Get the Qt Source Code

The current supported Qt version is 6.8.3.

  - In [here](https://download.qt.io/official_releases/qt/), in 6.8/6.8.3/submodules, download qtbase-everywhere-src-6.8.3.zip
  - Extract its contents and move them to **acad/qt5_src**

Alternatively contact the developer below to obtain source code.

## Build Qt

From the Visual Studio Start Menu folder, start a x64 developer terminal.  Then use the commands below to configure a release build, build it and install it:

```
cd <YOUR checked out acad directory>
mkdir qt5_build
cd qt5_build
..\qt5_src\configure.bat -prefix ..\qt5_release -release -no-sql-sqlite -nomake examples -nomake tests
cmake --build . --parallel
cmake --install .
```

You should repeat for a debug version of Qt; this is useful to prevent MSVC errors if you build your application in Debug.
    
```
..\qt5_src\configure.bat -prefix ..\qt5_debug -debug -redo
cmake --build . --parallel
cmake --install .
```

## Build ACAD

This section is based on [these instructions](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio).

  - Start MS Visual Studio
  - Select Open a Local Folder
    - Select your cloned **acad** folder
	- (You can close the cmake landing page if it comes up)
  - By default, **x64-Debug** configuration will be selected
    - Select the dropdown and \<Manage Configurations>
    - Add an **x64-Release** configuration 
  - Select \<Build>\<Build All> or CTRL+B to build

### Switch to Debug or Release build

  - In the release type pulldown, select either **x64-Release** or **x64-Debug**
  - Select \<Project>\<Delete Cache and Reconfigure>
  - Select \<Build>\<Build All> or CTRL+B to build

# Creating an Installer

ACAD uses the Qt Installer Framework to create its installer.  There are a couple of batch files to help with the creation.

* From www.qt.io online instller, install the Qt Installer Framework (currently version 4.10)
* Update CMakeLists.txt with the new version number
* Update installer/config/config.xml with the new version number
* Update installer/packages/****/meta/package.xml with the new version  number and date
* Update docs/User Guide.docx with the new version number, then print it to update User Guide.pdf
* Complete the release build as above
* Run the build. Open each example file in turn and re-save in order to update their native version.
* Open a powershell in the source installer directory
* Run .\createinstall.bat
* AV scan the installer and save to the installer directory

The installer acad_installer.exe will be created in the installer directory.

The createinstall.bat script uses the cleaninstall.bat script to empty the package directories before copying the latest build files over.  You can use cleaninstall.bat directly to clear the package directories (for example, before making a git commit).

# Contribution guidelines

Please contact arnie@madeupmodels.com if you would like to contribute, or if you require a copy of the Qt source code.

# Code Guidelines

Code formatting follows LLVM and can be re-applied using clang-format:

clang-format -style="{BasedOnStyle: llvm, ColumnLimit: 0}"  -i *.cpp *.h

Code style is somewhat mixed; [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) is a good start.

# Version History

3.0 First open-sourced version

3.1 Rib support tabs made fixed-width.  Width can be configured and marker squares for the rib outline are added every 10mm.

3.3 Add DXF export. New icons. (Restructure wing build sequence to support grouping in dxf exports; remove redundant spacer code.)

3.4 Girdiring tool tab.  Rib support tabs no longer cause issues with rib lightening holes and elements. Geodetics are no longer drawn reversed which was a problem when elements were included.

3.5 Add new wing sheeting jig type SJ2.

3.6 Update to Qt 6.8.1 and move to github
