#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
int main(int argc, const char * argv[]) {
int k=0;
int pipeNumber=0;
int howMany=1;
howMany=atoi(argv[1]);
int fd[howMany][2];
int fd2[howMany][2];
int fd3[howMany][2];
char message[256];
//char message2[256];
char str[50];
for (int i = 0; i < howMany; i++) {
		pipe(fd[i]);		// Pipes from input to Mappers
		pipe(fd2[i]); 		// Pipes from mappers to reducers
	}
for (int i = 0; i < howMany; i++) {
		pipe(fd3[i]); 		// Pipes among reducers
	}
if (argc==3) { //MAPPER PART
	for (int i = 0; i < howMany; i++) {
			//CHILDS
			if(fork() != 0){
							dup2(fd[i][0],0);
							for (int j = 0; j < howMany; j++){
										close(fd[j][1]);
										close(fd[j][0]);
									}
							
							snprintf(str, 50, "%d", i); //Send ID.
							execl(argv[2], "WC_Mapper", str, (char *)NULL);
		}
	}
	
//PARENT
	 while (fgets(message,256, stdin)) {
				pipeNumber=k%howMany;
				// Write into related pipe.
				write(fd[pipeNumber][1], message,strlen(message));
				k++;
	}
	for (int i = 0; i < howMany; i++) {
		close(fd[i][0]);
		close(fd[i][1]);
	}
	for (int i = 0; i < howMany; i++) {
	wait(NULL);
	}
}else if (argc==4) { //REDUCER PART

	for (int i = 0; i < howMany; i++) {
			//CHILDS
			if(fork() != 0){
							for (int j = 0; j < howMany; j++){
									if (j!=i) {
										close(fd2[j][1]);
										close(fd2[j][0]);
									}else{
										close(fd2[j][1]);
									}
									for (int i3 = 0; i3 < howMany; i3++) {
										close(fd[i3][0]);
										close(fd[i3][1]);
									}
							}
							dup2(fd2[i][0],0);
							close(fd2[i][0]);
								if (i!=0 && i!=(howMany-1)) {
									dup2(fd3[i-1][0],2);
									dup2(fd3[i][1],1);
								}else if (i==0 && howMany != 1 ) {
									dup2(fd3[i][1],1);
								}else if(howMany != 1){
									dup2(fd3[i-1][0],2);
								}
							for (int i2 = 0; i2 < howMany; i2++) {
								close(fd3[i2][0]);
								close(fd3[i2][1]);
							}
							snprintf(str, 50, "%d", i);
							execl(argv[3], "WC_Reducer", str, (char *)NULL);
		}
	}

		for (int i = 0; i < howMany; i++) {
			//CHILDS
			if(fork() != 0){
							for (int j = 0; j < howMany; j++){
									if (j!=i) {
										close(fd[j][1]);
										close(fd[j][0]);
										close(fd2[j][1]);
										close(fd2[j][0]);
									}else{
										close(fd[j][1]);
										close(fd2[j][0]);
									}
							}
							for (int j1 = 0; j1 < howMany; j1++) {
								close(fd3[j1][0]);
								close(fd3[j1][1]);
							}
							dup2(fd[i][0],0);
							close(fd[i][0]);
							dup2(fd2[i][1],1);
							close(fd[i][1]);
							snprintf(str, 50, "%d", i); //Send ID.
							execl(argv[2], "WC_Mapper", str, (char *)NULL);
		}
	}


	 while (fgets(message,256, stdin)) {
//PARENT
				pipeNumber=k%howMany;
				// Write input string and close writing end of pipe.
				write(fd[pipeNumber][1], message,strlen(message));
				k++;
	}
	for (int i = 0; i < howMany; i++) {
		close(fd[i][1]);
		close(fd[i][0]);
		close(fd2[i][1]);
		close(fd2[i][0]);
		close(fd3[i][1]);
		close(fd3[i][0]);

	}
	for (int i = 0; i < howMany*2; ++i)
	{
		wait(NULL);
	}
}
return 0;
}
