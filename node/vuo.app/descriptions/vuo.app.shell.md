Runs a command in a Unix shell.

`Command` can either be the shell command itself (e.g., `echo hi`), or it can be a path to a shell script or a command-line executable.

The command runs in a sandbox that permits accessing the network, and reading and writing files in the system's temporary folder (`/private/tmp/`).  It prohibits reading and writing files anywhere in the user's home folder.  For that reason, this node can't run most Mac apps with a graphical user interface — use the [Launch App](vuo-node://vuo.app.launch) node for Mac apps.

This node waits for the command to finish executing, then provides its standard output and standard error streams (the text `Output` port) and its exit code (`Status`).

By default, this node uses BASH to interpret the command.  You can use another interpreter by starting the script with a line containing `#!` and the path to the interpreter.  For example, to use [AppleScript](https://developer.apple.com/library/archive/documentation/AppleScript/Conceptual/AppleScriptLangGuide/introduction/ASLR_intro.html):

    #!/usr/bin/osascript
    display dialog "Hello from AppleScript!"

Or to use JavaScript:

    #!/usr/bin/osascript -l JavaScript
    console.log("Hello from JavaScript!")

Or Python:

    #!/usr/bin/python
    print "Hello from Python!"
