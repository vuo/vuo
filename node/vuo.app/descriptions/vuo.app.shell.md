Runs a command in a Unix shell.

`Command` can either be the shell command itself (e.g., `echo hi`), or it can be a path to a shell script or a command-line executable.

The command runs in a sandbox that permits accessing the network, and reading and writing files in the system's temporary folder (`/private/tmp/`).  It prohibits reading and writing files anywhere in the user's home folder.  For that reason, this node can't run most Mac apps with a graphical user interface — use the [Launch App](vuo-node://vuo.app.launch) node for Mac apps.

This node waits for the command to finish executing, then provides its standard output and standard error streams (the text `Output` port) and its exit code (`Status`).

## Script interpreters
By default, this node uses BASH to interpret the command.  You can use another interpreter by starting the script with a line containing `#!` and the path to the interpreter.

### Languages included with macOS
[AppleScript](https://developer.apple.com/library/archive/documentation/AppleScript/Conceptual/AppleScriptLangGuide/introduction/ASLR_intro.html):

    #!/usr/bin/osascript
    display dialog "Hello from AppleScript!"

[JavaScript](https://developer.mozilla.org/en-US/docs/Web/JavaScript):

    #!/usr/bin/osascript -l JavaScript
    console.log("Hello from JavaScript!")

### Additional languages
You can install [Homebrew](https://brew.sh), then use Homebrew to install other language interpreters, and use them with Vuo:

[CLISP](https://clisp.sourceforge.io):

    #!/opt/homebrew/opt/clisp/bin/clisp
    (print "Hello from Lisp!")

[Gravity](http://gravity-lang.org):

    #!/opt/homebrew/opt/gravity/bin/gravity
    func main() {
        System.print("Hello from Gravity!");
        return true;
    }

[Haskell](https://haskell.org):

    #!/usr/bin/env PATH=/opt/homebrew/opt/ghc/bin runhaskell
    main = putStrLn "Hello from Haskell!"

[Lua](https://lua.org):

    #!/opt/homebrew/opt/lua/bin/lua
    print "Hello from Lua!"

[node.js](https://nodejs.org):

    #!/opt/homebrew/opt/node/bin/node
    console.log("Hello from node.js!");

[OCaml](https://ocaml.org):

    #!/usr/bin/env PATH=/opt/homebrew/opt/ocaml/bin ocaml
    print_string "Hello from OCaml!";;

[PHP](https://php.net):

    #!/opt/homebrew/opt/php/bin/php
    <?php
    echo "Hello from PHP!";

[Python 3](https://python.org):

    #!/opt/homebrew/opt/python3/bin/python3
    print("Hello from Python!")

[Ruby](https://ruby-lang.org):

    #!/opt/homebrew/opt/ruby/bin/ruby
    print "Hello from Ruby!"

[Wren](https://wren.io)

    #!/opt/homebrew/opt/wren-cli/bin/wren_cli
    System.print("Hello from Wren!")
