#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <mpi.h>

int main(int argc, char* argv[]){
	int my_rank, proc_count;
	FILE *fp; //file pointer
	char buffer[0xFFFFF];
	int i=0, ii, size, startPointer, endPointer, sendbufArraySize, sizeSendBuffer;
	char *sendbuf = NULL, *recvbuf;
	int sizeTag=1, dataTag=2;
	int localCount=0, globalCount=0, letterCount=0;
	int minLetter=0, maxLetter=0;
	char filePath[0xFF];
	char* endPoint;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_count);

	//master node section
	if(my_rank==0){
		printf("Text file path: ");
		scanf("%s", &filePath);
		fp = fopen(filePath, "r");
		if(!fp){
			printf("Error: Text file not found.\n");
			exit(0);
		}
		printf("Minumum letter per word: ");
		scanf("%d", &minLetter);
		if(minLetter<=0){
			printf("Error: Invalid minumum letter per word.\n");
			exit(0);
		}
		printf("Maximum Letter per word: ");
		scanf("%d", &maxLetter);
		if(maxLetter<=minLetter){
			printf("Error: Invalid maximum letter per word.\n");
			exit(0);
		}
		fseek(fp, 0,SEEK_END);
		size = ftell(fp);
		rewind(fp);
		while(!feof(fp)){
   			fscanf(fp, "%c", &buffer[i]);
   			i++;
		}
		buffer[i-1]= '\0'; //Add null terminator to the end of char array
		//printf("%s\n", buffer); //debug purpose only
		fclose(fp);

		startPointer = 0;
		if(proc_count != 1)
			endPointer = size/(proc_count-1);
		else{
			printf("\x1B[31mError: Insufficient slave nodes\n");
			exit(0);
		}
		printf("%d\n\n", size); //debug purpose
		for(i=1; i<proc_count; i++){
			endPointer--;
			while(buffer[endPointer+1] != ' '){
				if(buffer[endPointer+1] == '\t')
					break;
				if(buffer[endPointer+1]!= '\0')
					endPointer++;
				else
					break;
			}
			sendbufArraySize = endPointer+1 - startPointer;
			if(sendbuf == NULL)
				sendbuf = malloc(sendbufArraySize+1); //+1 for null terminator
			else
				sendbuf = realloc(sendbuf, sendbufArraySize+1); //+1 for null terminator
			for(ii=0; ii<sendbufArraySize; ii++){
				sendbuf[ii] = buffer[startPointer+ii];
			}
			sendbuf[sendbufArraySize] = '\0';
			printf("%s\n", sendbuf);
			sizeSendBuffer = sendbufArraySize+1;
			MPI_Send(&sizeSendBuffer, 1, MPI_INT, i, sizeTag, MPI_COMM_WORLD); //send size of data
			MPI_Send(sendbuf, sizeSendBuffer, MPI_CHAR, i, dataTag, MPI_COMM_WORLD);

			endPointer++;
			startPointer = endPointer;
			
			if(proc_count-(i+1)!= 0){
				endPointer = endPointer+((size-endPointer)/(proc_count-(i+1)));
			}
		}

	}
	//end of master node section
		
	//slave nodes section
	if(my_rank!=0){
		MPI_Recv(&sendbufArraySize, 1, MPI_INT, 0, sizeTag, MPI_COMM_WORLD, &status);
		if(sendbufArraySize-1 != 0){
			recvbuf = malloc(sendbufArraySize);
			MPI_Recv(recvbuf, sendbufArraySize, MPI_CHAR, 0, dataTag, MPI_COMM_WORLD, &status);
			printf("Process %d received %d data: %s\n", my_rank, sendbufArraySize-1, recvbuf);
			i=0;
			while(recvbuf[i]==' ' || recvbuf[i] == '\t')
				i++;
			while(recvbuf[i]!= '\0'){
				if(recvbuf[i]==' ' || recvbuf[i]=='\t'){
					if(letterCount>=3)
						localCount++;
					letterCount=0;
					i++;
					continue;
				}
				else if(!ispunct(recvbuf[i])){
					if(!isdigit(recvbuf[i])){
						if(recvbuf[i]!= '\t')
							letterCount++;
					}
				}
				i++;
				if(recvbuf[i] == '\0'){
					if(letterCount>=3)
						localCount++;
					letterCount=0;
				}
			}
		}
		else
			printf("Process %d received no data\n", my_rank);
		
		printf("Process %d has %d words\n", my_rank, localCount);
	}
	//end of slave nodes section

	MPI_Finalize();
}

