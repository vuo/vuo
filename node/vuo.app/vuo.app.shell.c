/**
 * @file
 * vuo.app.shell node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <sys/errno.h>
#include <sys/stat.h>

VuoModuleMetadata({
	"title" : "Execute Shell Command",
	"keywords" : [
		"Unix", "cli", "command line", "Terminal", "console", "script", "system", "text code",
		"BASH", "AppleScript", "JavaScript", "Perl", "PHP", "Python", "Ruby", "cURL",
		"Lisp", "CLISP", "Gravity", "Haskell", "Lua", "node.js", "OCaml", "Wren",
		"``", "$()",
	],
	"version" : "1.0.3",
	"node" : {
		"exampleCompositions" : [ "ShowTextBanner.vuo" ]
	}
});

void nodeEvent(
	VuoInputData(VuoText, {"default":"whoami","isCodeEditor":true}) command,
	VuoInputEvent({"data":"command","hasPortAction":true}) commandEvent,
	VuoOutputData(VuoText) output,
	VuoOutputData(VuoInteger) status)
{
	if (VuoText_isEmpty(command))
		return;

	// Write the command to a file, then run it in a sandboxed shell
	// (safer than trying to escape the command and insert it into the sandbox command line).
	char *tempPath = strdup("/private/tmp/vuo-app-shell-XXXXXX");
	int tempFd = mkstemp(tempPath);
	fchmod(tempFd, 0700);
	write(tempFd, command, strlen(command));
	close(tempFd);

	const char *sandboxRules = VUO_STRINGIFY(
		(version 1)
		(deny default)
		(allow process-fork)
		(allow process-exec)
		(allow network*)
		(allow sysctl-read)
		(allow file-read* (require-any
			(literal "/")
			(literal "/dev/null")
			(literal "/dev/random")
			(literal "/dev/urandom")
			(literal "/private")
			(subpath "/Applications")
			(subpath "/Library")
			(subpath "/System")
			(subpath "/bin")
			(subpath "/etc")
			(subpath "/opt")
			(subpath "/private/etc")
			(subpath "/private/tmp")
			(subpath "/private/var")
			(subpath "/tmp")
			(subpath "/usr")
			(subpath "/var")
		))
		(allow file-write* (require-any
			(literal "/dev/null")
			(subpath "/private/tmp")
			(subpath "/private/var/folders")
			(subpath "/tmp")
		))

		// So AppleScript can pop up a window.
		(allow mach-lookup)
		(allow mach-register)
		(allow iokit-open)
		(allow ipc-posix-shm)
	);

	// Wrap it in `/bin/bash -c \"command …"` to support executables,
	// BASH scripts without a hashbang, and other scripts with a hashbang.
	char *sandboxCommand = VuoText_format("(cd / ; /usr/bin/sandbox-exec -p '%s' /bin/bash -c \"command %s\") 2>&1", sandboxRules, tempPath);
	*output = NULL;
	errno = 0;
	FILE *fp = popen(sandboxCommand, "r");
	free(sandboxCommand);
	if (!fp)
	{
		VUserLog("Error executing command: %s", strerror(errno));
		return;
	}

	size_t bytesAllocated = 1024;
	size_t bytesRead = 0;
	char *data = malloc(bytesAllocated);
	while (1)
	{
		bytesRead += fread(data + bytesRead, 1, bytesAllocated - bytesRead, fp);
		if (feof(fp))
		{
			data[bytesRead] = 0;
			break;
		}
		else
		{
			bytesAllocated *= 2;
			data = realloc(data, bytesAllocated);
		}
	}
	while (!feof(fp));

	*output = VuoText_makeWithoutCopying(data);

	int pcloseStatus = pclose(fp);
	*status = WIFEXITED(pcloseStatus) ? WEXITSTATUS(pcloseStatus) : -1;

	unlink(tempPath);
	free(tempPath);
}
