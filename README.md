These instructions are for building Vuo Base, Vuo Compiler, Vuo Renderer, Vuo Runtime, Vuo Editor, and the built-in Vuo Nodes and Types from source code.

Vuo's source code is available so you can learn about how Vuo works, play with it, and maybe even help develop Vuo.

You ***do not*** need to build Vuo from source if you want to:

   - **run the Vuo editor application**.  The Vuo editor application is available as a [separate download](https://vuo.org/download).
   - **develop an application that uses Vuo**.  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](https://api.vuo.org) under the section "Developing Applications that use Vuo".
   - **develop nodes and types for Vuo**.  Instead, [download the Vuo SDK](https://vuo.org/download), and follow the instructions on [api.vuo.org](https://api.vuo.org) under the section "Developing Node Classes and Port Types".


# Translating Vuo
Vuo uses the [Qt Framework's translation system](https://doc.qt.io/qt-5/qtlinguist-index.html) in its source code, and POEditor to manage translations online.  Sign up here to help translate Vuo: <https://poeditor.com/join/project/KJuvGOpptm>


# Building Vuo on macOS


## Install required dependencies

### Xcode

Install a recent version of [Xcode](https://developer.apple.com/xcode/).

Launch Xcode and accept the license agreement.

Install Command Line Tools for Xcode:

   - Go to https://developer.apple.com/download/more/
   - In the search box, enter your Xcode version.
   - In the search results, locate and download the Command Line Tools.
   - Open the downloaded package and follow the prompts.

### Homebrew

Install Homebrew:

```sh
ruby -e "$(curl -fsSL https://raw.github.com/mxcl/homebrew/go/install)"
brew doctor
```

Review the results of `brew doctor` and fix any problems it finds, then install the dependencies:

   - `cd` to the Vuo source code folder.
   - `brew bundle`

### Conan

   - `pip3 install --user conan==1.59.0`
   - Add the directory in which `conan` was just installed to the shell's command search path:
      - `python3 -c 'import site; print(site.getuserbase())'`
      - Take the output of that command and append `/bin` to get the directory containing `conan` (e.g. `/Users/me/Library/Python/3.10/bin`).
      - In `~/.zshrc`, `~/.profile`, or similar, add that directory to your shell's `PATH` variable.
   - `cd` to the Vuo source code folder.
   - `mkdir build`
   - `cd build`
   - On macOS:
      - `conan config install https://vuo.org/sites/default/files/conan-macos.zip`
      - Conan may output `WARN: Remotes registry file missing, creating default one` — that's OK.
   - On Linux:
      - `conan config install https://vuo.org/sites/default/files/conan-linux.zip`
   - `conan install ..`

### TeX (optional)

To build the user manual (PDF, HTML, or MacHelp variants), install TeX from https://www.tug.org/mactex/ .

### Behat (optional)

To be able to run `TestBuildSystem`, install Behat following the instructions on <https://docs.behat.org/en/latest/quick_start.html#installation>. Add its directory to your shell's `PATH` variable.

## Build Vuo using Qt Creator

Install [Qt Creator](https://download.qt.io/official_releases/qtcreator/).

Launch Qt Creator.

   - In Preferences:
      - In Kits:
         - In the Kits tab, select the default kit and remove the content of "CMake Configuration"
         - In the Qt Versions tab:
            - Delete all the existing versions
            - Click Add, press Command-Shift-G and enter `~/.conan`, then navigate to `data/qt/<latest version>/vuo/stable/package/<package id>/bin/qmake`
            - (You'll see "Qt version is not properly installed" in the dialog, and "Cannot read …/qmake.conf" in the General Messages tab — those errors are safe to ignore.)
      - In Environment > Locator:
         - Uncheck the Default checkboxes for:
            - All Included C/C++ Files
            - Files in Any Project
         - Add a new directory filter:
            - Name: `Vuo trunk`
            - Directories: pick your working copy folder
            - File pattern: `*.h,*.hh,*.c,*.cc,*.m,*.mm,*.md,*.glsl,*.vs,*.fs,*.qml,*.ui,*.qrc,*.txt,*.cmake,*.feature,*.php`
            - Exclusion pattern: `*/.git/*,*/build/*`
      - In C++, select the File Naming tab:
         - Change Header suffix to "hh"
         - Change Source suffix to "cc"
         - Uncheck the "Lower case file names" checkbox
      - In Build & Run:
         - In General, change "Default build directory" to `build`
   - Open `CMakeLists.txt`
      - In the list of kits, click Details, and uncheck all the targets except "Default"
      - Click Configure Project
   - In Projects (the wrench near the top left):
      - Build > Build Steps > Build, click Details, then set Tool Arguments to `-j8` (or however many cores your processor has)
      - Run > Run, uncheck "Add build library search path to DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH"
      - (optional, to colorize build steps) Build > Build Environment, click Details, click Add, and set `CLICOLOR_FORCE` to `1`
   - In the target selector (the computer icon near the bottom left), select VuoEditorApp
   - In the Search Results panel at the bottom of the window, set Exclusion Pattern to `**/build/**`

## Build Vuo from the command line

As an alternative to using Qt Creator (above), you can build from the command line.

First, generate the makefiles:

```sh
mkdir build
cd build
cmake ..
```

Build:

```sh
make -j8
```

Run Vuo Editor:

```sh
make go
```

…or open the `build/bin` folder in Finder and double-click `Vuo.app`.

Run the command-line tools:

```sh
bin/vuo-compile --help
bin/vuo-link --help
bin/vuo-debug --help
```

Optionally, build and run the tests:

```sh
make -j8 VuoTest

# Consider adding this alias to your ~/.bashrc :
alias ctest='ctest -j8 --output-on-failure --progress'

ctest                                    # Run all tests (takes a few hours)
ctest -N                                 # List the names of all tests
ctest -R …                               # Run tests whose name matches the specified regex (case-sensitive)
ctest -R vuo.math.keep.average           # Run tests for a node
ctest -R VuoAnchor                       # Run tests for a type
ctest -R PlayBluesOrgan                  # Run tests for an example composition
ctest -R TestModules.testType:VuoAnchor  # Run a single test datum
```

Optionally, build the documentation:

```sh
cmake .. -DVUO_DOCUMENTATION=ON
make -j8 VuoManualPDF_en_highContrast    # Build the manual as a PDF
make -j8 VuoManualHTML                   # Build the manual as HTML
make -j8 VuoManualMacHelp                # Build the manual as a MacHelp Book
make -j8 VuoDoxygen                      # Build the SDK documentation
```

## Developer shortcuts
Vuo's build system includes some shortcuts to make development more efficient.

### Compiler development mode
Allows you to make changes to VuoBase and VuoCompiler without rebuilding all nodes and types (but you're responsible for keeping track of whether nodes and types need to be rebuilt).  Saves several minutes per edit-compile-test cycle.

```sh
cmake .. -DVUO_COMPILER_DEVELOPER=ON
cmake .. -DVUO_COMPILER_DEVELOPER=OFF
```

### Node development mode
Allows you to make changes to nodes, types, and libraries without rebuilding the Vuo.framework cache.  Saves about 30 seconds per edit-compile-test cycle (but compositions take longer to start up initially).

```sh
cmake .. -DVUO_NODE_DEVELOPER=ON
cmake .. -DVUO_NODE_DEVELOPER=OFF
```
