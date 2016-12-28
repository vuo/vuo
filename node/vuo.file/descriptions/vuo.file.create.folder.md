Makes a new folder.

   - `Create at URL` — The file path of the folder to be created. For example, "MyFolder" creates a folder called MyFolder in the folder that the current composition is in. "~/Documents/MyOtherFolder" creates a folder called MyOtherFolder in the Documents folder. If the composition is not saved yet, folders are created on the Desktop. When this port receives an event, the folder is created (if it doesn't already exist).

If the URL contains a series of folders that don't already exist, then all of the folders will be created. For example, if the URL is "a/b/c" and folders "b" and "c" don't already exist, they will both be created.

If the folder doesn't already exist and can't be created, an error message is logged to Console.
