//*********************************************************
//
// Cody Pinkston
// Operating Systems
// Writing Your Own Shell: hfsh26
//
//*********************************************************


//*********************************************************
//
// Includes and Defines
//
//*********************************************************
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>


std::string STR_PROMPT = "hfsh26> ";
const std::string STR_EXIT = "exit";
const std::string STR_CD = "cd";
const std::string STR_PATH = "path";
const std::string STR_ERROR = "An error has occurred\n";




//*********************************************************
//
// Extern Declarations
//
//*********************************************************
// Buffer state. This is used to parse string in memory...
// Leave this alone.
extern "C"{
    extern char **gettoks();
    typedef struct yy_buffer_state * YY_BUFFER_STATE;
    extern YY_BUFFER_STATE yy_scan_string(const char * str);
    extern YY_BUFFER_STATE yy_scan_buffer(char *, size_t);
    extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
    extern void yy_switch_to_buffer(YY_BUFFER_STATE buffer);
}


//*********************************************************
//
// Namespace
//
// If you are new to C++, uncommment the `using namespace std` line.
// It will make your life a little easier.  However, all true
// C++ developers will leave this commented.  ;^)
//
//*********************************************************
// using namespace std;


//*********************************************************
//
// Function Prototypes
//
//*********************************************************
std::string buildPath(const std::string& cmd, const std::vector<std::string>& path);
int execCmd(std::vector<std::vector<std::string>>& cmds, const std::vector<std::string>& path, char **toks, char*& outFile);
int pathCmd(std::vector<std::string>& path, char **toks);
int checkRedirection(char **toks, char*& outFile);
int parseParallel(char **toks, std::vector<std::vector<std::string>>& cmds);
std::vector<char*> convertVector(std::vector<std::string>& cmd);
//*********************************************************
//
// Main Function
//
//*********************************************************
int main(int argc, char *argv[])
{
    /* local variables */
    int ii;
    char **toks;
    int retval;
    char linestr[256];
    YY_BUFFER_STATE buffer;

    // Where I should add my vars
    FILE *infp = stdin;
    std::vector<std::string> path;
    char* outFileName = nullptr;
    std::vector<std::vector<std::string>> cmds;
    int interactive = 1;

    /* initialize local variables */
    ii = 0;
    toks = NULL;
    retval = 0;

    path.push_back("/bin"); // set the standard path to /bin

    // Setup for the file pointer when program first runs
    if(argc == 2){
    	infp = fopen(argv[1], "r");
	interactive = 0;
	if(infp == NULL){
		std::cerr << STR_ERROR;
		exit(1);
	}
    } else if(argc > 2){
	std::cerr << STR_ERROR;
	exit(1);
    }


	// Print once before loop, then at each end of loop 
	if (interactive){
	    	std::cout << STR_PROMPT;	
	}

    /* main loop */
    while(fgets(linestr, 256, infp)){
        // make sure line has a '\n' at the end of it
        if(!strstr(linestr, "\n"))
            strcat(linestr, "\n");


        /* get arguments */
        buffer = yy_scan_string(linestr);
        yy_switch_to_buffer(buffer);
        toks = gettoks();
        yy_delete_buffer(buffer);

        if(toks[0] != NULL){
	    // clearing variables each run
	    cmds.clear();
	    outFileName = nullptr;
	
	    if(!strcmp(toks[0], STR_EXIT.c_str())) {
	   	// Built in exit commmand
		if(toks[1] == NULL){
		    break;
		} else {
		    std::cerr << STR_ERROR;
		}
	    } else if (!strcmp(toks[0], STR_CD.c_str()) ){
	    	// Built in cd command
		if (toks[1] == NULL) {
			std::cerr << STR_ERROR;
		} else if (toks[2] != NULL) {	
			std::cerr << STR_ERROR;
		} else {
			if (chdir(toks[1]) != 0) {	
				std::cerr << STR_ERROR;
			}
		}
	    } else if(!strcmp(toks[0], STR_PATH.c_str())){
	    	// Built in Path command
		int checkPath = pathCmd(path, toks);
		if(checkPath){
			std::cerr << STR_ERROR;
		}
	    } else {
		// Tries to run whaterver is left as a command
		parseParallel(toks, cmds);
		if(execCmd(cmds, path, toks, outFileName)){
			std::cerr << STR_ERROR;
		} 		 
	    }
        }
	//Print Prompt
	if (interactive){
	    	std::cout << STR_PROMPT;	
	}
    }

    /* return to calling environment */
    return( retval );
}


// Searchs to build the path to run commands if not found will throw error
std::string buildPath(const std::string& cmd, const std::vector<std::string>& path){
	for(const auto& p : path){
		std::string built_path = p + "/" + cmd;

		if(!(access(built_path.c_str(), X_OK))){
			return built_path;
		}
	}
	return "";
}

// Main function to exec commands, handles forks, and parallel commands
int execCmd(std::vector<std::vector<std::string>>& cmds, const std::vector<std::string>& path, char **toks, char*& outFile){
	std::string fullCmd;
	int fd;
	size_t size = cmds.size();
	pid_t fork_pids[size];


	// Skip when there are no commands
	if(size == 0){
		return 0;
	}

	for(int i=0; i<size; i++){
		// skips empty commands with multiple commands input
		if(cmds[i].empty()){
			continue;
		}

		pid_t pid = fork();

		// Child
		if(pid == 0) {
		char* cmdOutFile = nullptr;
		std::vector<char*> args = convertVector(cmds[i]);

		// Checks if the line has redirection
		if(checkRedirection(args.data(), cmdOutFile)){
			std::cerr << STR_ERROR;
			exit(1);
		}

		fullCmd = buildPath(args[0] ,path);
		if(fullCmd.empty()){ // if path returns empty the cmd could not be found
			std::cerr << STR_ERROR;
			exit(1);
		}
			//File redirection with error checking
			if(cmdOutFile != nullptr){
				if((fd = open(cmdOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
					std::cerr << STR_ERROR;
					exit(1);
				} else {
					if((dup2(fd, STDOUT_FILENO)) == -1){
						exit(1);
					}

					if((dup2(fd, STDERR_FILENO)) == -1){
						exit(1);
					}
					close(fd);
				}		
			}
			// call to run a non built in command
			execv(fullCmd.c_str(), args.data());
			exit(1);
		} 
	
	}
	
	// Parent waits for all children
	for(int i=0; i<size; i++){
		int status;
		wait(&status);
	}

	return 0;
}

// Command that manages the path built in command
int pathCmd(std::vector<std::string>& path, char **toks){
	path.clear();
	int i = 1;
	while(toks[i] != NULL){
		path.push_back(toks[i]);
		i++;
	}
	return 0;
}


// Command that checks for and correct the array to handle redirection
int checkRedirection(char **toks, char*& outFile){
	outFile = nullptr;

	for(int i=0; toks[i] != NULL; i++){
		if(strcmp(toks[i], ">") == 0){
			if((toks[i + 1] != NULL) && (toks[i+2] == NULL) && (i>0)){ // Checks that > is in the right formate
				outFile = toks[i+1];
				toks[i] = NULL; // Will terminate before reaching the > symbol
				return 0;
			} else {		
				return -1;
			}	
		}
	}
	return 0;
}

// Checks for &, and splits each individual command into its own array so it can be forked on its own
int parseParallel(char **toks, std::vector<std::vector<std::string>>& cmds){
	std::vector<std::string> temp;
	for(int i = 0; toks[i] != NULL; i++){
		if((strcmp(toks[i], "&")) == 0){
			
			if(!temp.empty()){    //Will only insert when temp has elements, handles case of & with no command
				cmds.push_back(temp);
				temp.clear();
			}
		} else {
			temp.push_back(toks[i]);
		}
	}

	// If there is anything left we need to make sure to put in in vector as well
	if(!(temp.empty())){
		cmds.push_back(temp);
	}

	return 0;
}




// Converting between Types
std::vector<char*> convertVector(std::vector<std::string>& cmd){
	std::vector<char*> temp;
	for(const auto& c : cmd){
		temp.push_back(const_cast<char*>(c.c_str()));
	}
	temp.push_back(nullptr);

	return temp;
}
