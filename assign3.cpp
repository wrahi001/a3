#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::algorithm;

// split the input line into an array
void parse(char *line, char **argv)
{
	while (*line != '\0') {    /* if not the end of line ....... */
		while (*line == ' ' || *line == '\t' || *line == '\n')
		*line++ = '\0';  /* replace white spaces with 0    */
		*argv++ = line;    /* save the argument position  */
		while (*line != '\0' && *line != ' ' &&
		*line != '\t' && *line != '\n')
		line++;       /* skip the argument until ...    */
	}
	*argv = '\0';        /* mark the end of argument list  */
}

// execute the command that's been split up into an array
bool execute(char **argv)
{
	pid_t pid;
	int status;

	if ((pid = fork()) < 0) {  /* fork a child process     */
		perror("fork() failed");
		exit(1);
	}
	else if (pid == 0) {    /* for the child process:      */
		if (execvp(*argv, argv) < 0) {  /* execute the command  */
			perror("execvp failed");
			exit(1);
		}
	}
	else {                /* for the parent:   */
		while (waitpid(pid, &status, 0) != pid)    /* wait for completion  */
		;
		if(WIFEXITED(status))
		{
			if(WEXITSTATUS(status) != 0)
			{
				return false;
			}
		}
		return true;
	}
}

bool runTest(string cmd, int mode = 0) {
	struct stat sb;
	bool statVal = false;
	string testArg = "";
	const char *c;
	
	if (mode == 0) { // example: test -e test.txt
		// get the argument
		testArg = cmd.substr(5, 3);

		// strip "test " and the argument from the command to get just the file/directory name
		if (testArg == "-e " || testArg == "-f " || testArg == "-d ") {
			c = cmd.substr(5 + 3).c_str();
		} else {
			c = cmd.substr(5).c_str();
		}
	} else { // example: [ -e test.txt ]
		// get the argument
		testArg = cmd.substr(2, 3);

		// strip "[ ," " ]," and the argument from the command to get just the file/directory name
		if (testArg == "-e " || testArg == "-f " || testArg == "-d ") {
			c = cmd.substr(2 + 3, cmd.size() - (2 + 3) - 2).c_str();
		} else {
			c = cmd.substr(2, cmd.size() - 2).c_str();
		}
	}
	
	// display whether the file/directory exists
	if (stat(c, &sb) == -1) {
		cout << "(False)" << endl;
		return false;
	} else {
		// display whether the file/directory is a file
		if (testArg == "-f ") {
			if ((sb.st_mode & S_IFMT) != S_IFREG) {
				cout << "(False)" << endl;
				return false;
			} else {
				cout << "(True)" << endl;
				return true;
			}
			// display whether the file/directory is a directory
		} else if (testArg == "-d ") {
			if ((sb.st_mode & S_IFMT) != S_IFDIR) {
				cout << "(False)" << endl;
				return false;
			} else {
				cout << "(True)" << endl;
				return true;
			}
		} else {
			cout << "(True)" << endl;
			return true;
		}
	}
}

int main() {
	string cmd = "";
	string currCmd; // stores a single command from the input line with potentially multiple commands
	char *argv[64];
	bool execVal = false;
	bool doBreak = false;
	bool inPO = false; // inside a precedence operator

	while (1) {

		// display the command prompt
		cout << "$ ";

		// read the input
		getline(cin, cmd);

		if (cmd == "exit") {
			cout << "Exiting..." << endl;
			break;
		}

		// reset things
		currCmd = "";
		doBreak = false;
		
		for (int i = 0; i < cmd.length() + 1; i++) {
			if (cmd[i] == ';' || cmd[i] == '&' || cmd[i] == '|' || cmd[i] == '#' || i == cmd.length()) {
				// trim leading and trailing spaces
				trim(currCmd);

				// skip to the next iteration if the command is blank after being trimmed
				if (currCmd == "") {
					continue;
				}

				// we've encountered the test command 0 (example: test -e test.txt)
				if (currCmd.substr(0, 5) == "test ") {
					execVal = runTest(currCmd);
				} // end test command 0
				// we've encountered the test command 1 (example: [ -e test.txt ])
				else if (currCmd.substr(0, 2) == "[ ") {
					execVal = runTest(currCmd, 1);
				} // end test command 1 
				else {
					char *currCmd2 = &currCmd[0];
					parse(currCmd2, argv);

					execVal = execute(argv);
				}

				if (cmd[i] == ';') { // process a ";" connector
					//if (!execVal) cout << "Execution failed for command: " << currCmd << "!" << endl;
				} else if (cmd[i] == '&') { // for a "&&" connector
					i++;
					if (!execVal) {
						//cout << "Execution failed for command: " << currCmd << "! Remaining command terminated (&&)." << endl;
						doBreak = true;
					}
				} else if (cmd[i] == '|') { // for a "||" connector
					i++;
					if (execVal) {
						//cout << "Execution succeeded for command: " << currCmd << "! Remaining command terminated (||)." << endl;
						doBreak = true;
					}
				} else if (cmd[i] == '#') { // for a comment "#"
					//if (!execVal) cout << "Execution failed for command: " << currCmd << "!" << endl;
					doBreak = true;
				} else if (i == cmd.length()) { // we've reached the end of the command. execute whatever's remaining
					//if (!execVal) cout << "Execution failed for command: " << currCmd << "!" << endl;
					doBreak = true;
				}

				// reset things
				currCmd = "";

				if (!doBreak) {
					continue;
				} else {
					break;
				}

			}

			// add the current character to the current command variable
			currCmd += cmd[i];
		}
	}

	return 0;
}
