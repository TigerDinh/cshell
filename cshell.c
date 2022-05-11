#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

char* colorTheme;

typedef struct {
	char *name;
	char *value;
} EnvVar;

typedef struct{
	EnvVar* arrEnvVar;
	int currSize;
	int maxSize;
} ListOfEnvVar;

typedef struct {
	char** arrayTokens;
	int currSize;
} UserInput;

typedef struct {
	char commandInput[100];
	time_t time;
	int returnValue;
} Command;

typedef struct {
	Command* arrayCommand;
	int currSize;
	int maxSize;
} CommandHistory;

void storeCommand(char** commandLine, CommandHistory* commandHistoryPtr, int* returnCode){
	// Resize if full
	if (commandHistoryPtr->currSize == commandHistoryPtr->maxSize){
		commandHistoryPtr->maxSize = (commandHistoryPtr->maxSize)+1;
		Command* tempPtr;
		if ((tempPtr = realloc(commandHistoryPtr->arrayCommand, commandHistoryPtr->maxSize * sizeof(Command)))){
			commandHistoryPtr->arrayCommand = tempPtr;
		}
		else{
			printf("Allocation failed\n");
			return;		
		}
	}
	
	// Storing return value, time, and command
	Command newCommand;
	newCommand.returnValue = *returnCode;
	strcpy(newCommand.commandInput, commandLine[0]);
	newCommand.time  = time(NULL);
	
	commandHistoryPtr->arrayCommand[commandHistoryPtr->currSize] = newCommand;
	commandHistoryPtr->currSize = commandHistoryPtr->currSize + 1;
	return;
}

int cshellLog(CommandHistory* commandHistoryPtr){
	for (int i = 0; i < commandHistoryPtr->currSize; i++){
		printf("%s", (asctime(localtime(&(commandHistoryPtr->arrayCommand[i]).time))));
		printf(" %s ", ((commandHistoryPtr->arrayCommand[i].commandInput)));
		printf("%d\n",(commandHistoryPtr->arrayCommand[i].returnValue));
	}
	return 0;
}

int cshellPrint(char** commandLine, int commandLineSize, ListOfEnvVar* listOfEnvVarPtr){
	if (commandLineSize < 2){
		return 0;
	}
	
	if (commandLineSize == 2){
		for (int i = 0; i < listOfEnvVarPtr->currSize; i++){
			if (!strcmp(commandLine[1],listOfEnvVarPtr->arrEnvVar[i].name)){				
				printf("%s\n", listOfEnvVarPtr->arrEnvVar[i].value);
				return 0;
			}
		}
	}

	for (int index = 1; index < commandLineSize; index++){
		printf("%s ", commandLine[index]);
	}
	printf("\n");
	
	return 0;
}

int cshellTheme(char** commandLine, int commandLineSize){
	if (commandLineSize != 2){
		printf("Please enter a color (red, green, or blue): \n");
		return 0;
	}
	
	if (!strcmp(commandLine[1], "red")){
		colorTheme = "\x1b[31m";
		printf("\x1b[31m");
	}
	else if (!strcmp(commandLine[1],"green")){
		colorTheme = "\x1b[32m";
		printf("\x1b[32m");
	}
	else if (!strcmp(commandLine[1], "blue")){
		colorTheme = "\x1b[34m";
		printf("\x1b[34m");
	}
	else{
		printf("Please enter a color (red, green, or blue): \n");
	}
	return 0;
}

int cshellVariableAssign(char** commandLine, int commandLineSize, ListOfEnvVar* listOfEnvVarPtr){
	char* variableName = strtok(commandLine[0], "=");
	char* variableValue = strtok(NULL, "=");
	
	// Checking for invalid commands of assigning variables
	if (variableValue == NULL){
		printf("Please enter variable value\n");
		return 0;
	}

	if (commandLineSize > 1){
		printf("Too many variable values\n");
		return 0;
	}
	
	
	//Resize if full
	if (listOfEnvVarPtr->currSize == listOfEnvVarPtr->maxSize){
		listOfEnvVarPtr->maxSize = (listOfEnvVarPtr->maxSize) + 2;
		listOfEnvVarPtr->arrEnvVar = realloc(listOfEnvVarPtr->arrEnvVar, (listOfEnvVarPtr->maxSize)*sizeof(EnvVar));
	}
	
	// Checking if the variableName already exist in arrEnvVar before reassigning or adding it into the array
	int alreadyExist = -1;
	for (int i = 0; i < listOfEnvVarPtr->currSize; i++){
		if (!strcmp(listOfEnvVarPtr->arrEnvVar[i].name, variableName)){
			alreadyExist = 0;
			listOfEnvVarPtr->arrEnvVar[i].value = variableValue;
		}
	}
	
	// Adding into arrEnvVar
	if (alreadyExist == -1){
		listOfEnvVarPtr->arrEnvVar[listOfEnvVarPtr->currSize].name = variableName;
		listOfEnvVarPtr->arrEnvVar[listOfEnvVarPtr->currSize].value = variableValue;
		listOfEnvVarPtr->currSize = listOfEnvVarPtr->currSize + 1;
	}

	return 0;
}

void createChildProcess(char* command){
	int fds[2];
	char output[256];

	pipe(fds);
	int fc = fork();

	if(fc == 0) {
		// Redirecting output to pipe
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		dup2(fds[1], STDERR_FILENO);
		close(fds[1]);
		
		execlp(command, command, NULL);
	}
	else{
		close(fds[1]);

		while (read(fds[0], output, 1) > 0){
			write(1, output, 1);
		}

		wait(NULL);
		close(fds[0]);
	}
	return;
}	

int cshellExit(){
	printf("Bye!\n");
	return 1;
}

int executeLine(char** commandLine, CommandHistory* commandHistoryPtr, int commandLineSize, ListOfEnvVar* listOfEnvVarPtr){
	int status = 0;
	int returnCode = 0;
	
	// If empty command
	if (commandLine[0] == NULL || commandLineSize == 0){
		return 0;
	}
	
	// Executing valid commands
	if (!strcmp(commandLine[0],"exit")){
		return cshellExit();
	}
	else if (!strcmp(commandLine[0],"print")){
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		return cshellPrint(commandLine, commandLineSize, listOfEnvVarPtr);
	}
	else if (!strcmp(commandLine[0],"log")){
		status = cshellLog(commandHistoryPtr);
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		return status;
	}
	else if (!strcmp(commandLine[0],"theme")){
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		return cshellTheme(commandLine, commandLineSize);
	}
	else if (!strcmp(commandLine[0],"ls") || 
	!strcmp(commandLine[0],"pwd") || 
	!strcmp(commandLine[0],"whoami")){
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		createChildProcess(commandLine[0]);
		return status;
	}
	else if (commandLine[0][0] == '$'){
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		return cshellVariableAssign(commandLine, commandLineSize, listOfEnvVarPtr); 
	}
	else{
		printf("Invalid command\n");
		returnCode = 1;
		storeCommand(commandLine, commandHistoryPtr, &returnCode);
		return status; 
	}
}

UserInput* parseLine(char* currLine){
	const char separator[10] = " \t\r\n";
	int increaseSizeBy = 5;
	int maxSize = 5;
	int currSize = 0;
	UserInput userInput;
	userInput.arrayTokens = malloc(sizeof(char*)* maxSize);
	userInput.currSize = 0;
	UserInput* userInputPtr = &userInput;
	char* token = strtok(currLine, separator);
	
	if (userInput.arrayTokens == NULL){
		printf("Allocation error");
		exit(8);
	}
	
	while (token != NULL){
		// Resize if full
		if (currSize == maxSize){
			maxSize = maxSize + increaseSizeBy;
			userInput.arrayTokens = realloc(userInput.arrayTokens, maxSize*sizeof(char*));
			if (userInput.arrayTokens == NULL){
				printf("Allocation error");
				exit(8);
			}
		}
		
		// Adding token to the arrayTokens
		userInput.arrayTokens[currSize] = token;
		currSize++;
		userInput.currSize = currSize;
		token = strtok(NULL, separator);
	}
	return userInputPtr;
}

char* getLine(){
	printf("\x1b[0m");
	size_t defaultSize = 5;
	char* line = malloc(sizeof(char)* defaultSize);
	
	if (line == NULL){
		printf("Allocation error");
		exit(8);
	}
	
	if (getline(&line, &defaultSize, stdin) == -1){
		printf("Reading line failed\n");
		return NULL;
	}
	printf("%s",colorTheme);
	return line;
}

int checkFileInDirectory(char* fileName){
	
	// Finding fileName in current directory
	int fds[2];
	char filesInDirectory[1000];

	pipe(fds);
	int fc = fork();

	if(fc == 0) {
		// Redirecting output to pipe
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		dup2(fds[1], STDERR_FILENO);
		close(fds[1]);
		
		execlp("ls", "ls", NULL);
		exit(EXIT_SUCCESS);
	}
	else{
		close(fds[1]);
		
		while (read(fds[0], filesInDirectory, 1000) > 0){
			
			// Parsing filesInDirectory
			const char separator[10] = "\n";
			int increaseSizeBy = 5;
			int maxSize = 5;
			int currSize = 0;
			char** arrayFileName = malloc(sizeof(char*)* maxSize);
			char* token = strtok(filesInDirectory, separator);
			
			if (arrayFileName  == NULL){
				printf("Allocation error");
				exit(8);
			}
			
			while (token != NULL){
				// Resize if full
				if (currSize == maxSize){
					maxSize = maxSize + increaseSizeBy;
					arrayFileName  = realloc(arrayFileName , maxSize*sizeof(char*));
					if (arrayFileName  == NULL){
						printf("Allocation error");
						exit(8);
					}
				}
				
				// Adding token to the arrayFileName
				arrayFileName[currSize] = token;
				currSize++;
				token = strtok(NULL, separator);
			}
			
			// Comparing fileName with the names of the files in current directory
			for (int i = 0; i < currSize; i++){
				if (!strcmp(fileName, arrayFileName[i])){
					return 1;
				}
			}
		}
	
		wait(NULL);
		close(fds[0]);
	}
	return 0;
}

void launchScriptMode(char* fileName){
	CommandHistory commandHistory;
	commandHistory.maxSize = 2;
	commandHistory.currSize = 0;
	commandHistory.arrayCommand =  malloc(sizeof(Command)*commandHistory.maxSize);
	
	ListOfEnvVar listOfEnvVar;
	listOfEnvVar.maxSize = 2;
	listOfEnvVar.currSize = 0;
	listOfEnvVar.arrEnvVar = malloc(sizeof(EnvVar)*listOfEnvVar.maxSize);
	
	UserInput* userInputPtr;
	char** testingArrayTokens;
	
	colorTheme = "\x1b[0m";
	
	// Reading each line of fileName and executing it
	FILE* chosenFile;
	chosenFile = fopen(fileName, "r");
	char* line = NULL;
	size_t length = 0;
	
	while (getline(&line, &length, chosenFile) != -1){
		userInputPtr = parseLine(line);
		testingArrayTokens = userInputPtr->arrayTokens;
		executeLine(userInputPtr->arrayTokens, &commandHistory, userInputPtr->currSize, &listOfEnvVar);
	}
	fclose(chosenFile);
	
	free(commandHistory.arrayCommand);
	free(listOfEnvVar.arrEnvVar);
	free(testingArrayTokens);
	return;
}

void launchShell(){
	char* line;
	UserInput* userInputPtr;
	int status = 0;
	
	CommandHistory commandHistory;
	commandHistory.maxSize = 2;
	commandHistory.currSize = 0;
	commandHistory.arrayCommand =  malloc(sizeof(Command)*commandHistory.maxSize);
	
	ListOfEnvVar listOfEnvVar;
	listOfEnvVar.maxSize = 2;
	listOfEnvVar.currSize = 0;
	listOfEnvVar.arrEnvVar = malloc(sizeof(EnvVar)*listOfEnvVar.maxSize);
	
	char** testingArrayTokens;
	
	colorTheme = "\x1b[0m";
	
	while(status != 1){
		printf("cshell$ ");
		line = getLine();
		userInputPtr = parseLine(line);
		testingArrayTokens = userInputPtr->arrayTokens;
		status = executeLine(userInputPtr->arrayTokens, &commandHistory, userInputPtr->currSize, &listOfEnvVar);
		
	}
	
	free(line);
	free(commandHistory.arrayCommand);
	free(listOfEnvVar.arrEnvVar);
	free(testingArrayTokens);
	return;
}

int main(int numOfArgument, char* argv[]){
	
	if (numOfArgument > 2){
		printf("Too many arguments\n");
		return 0;
	}
	
	// Launching Script Mode
	if (numOfArgument == 2){
		// Checking if file exist
		if (checkFileInDirectory(argv[1])){
			launchScriptMode(argv[1]);
		}
		else{
			printf("%s does not exist\n", argv[1]);
		}
		return 0;
	}
	
	launchShell();
	return 0;
}
