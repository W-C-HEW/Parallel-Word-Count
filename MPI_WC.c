#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char* argv[]){
	int my_rank, proc_count;
	FILE *fp; //file pointer
	char *buffer;
	buffer = malloc(0xFFFFFFFF);
	int i=0, ii, size, startPointer, endPointer, sendbufArraySize, sizeSendBuffer;
	char *sendbuf = NULL, *recvbuf;
	int sizeTag=1, dataTag=2;
	int localCount=0, globalCount=0, letterCount=0;
	int minLetter=0, maxLetter=0;
	char filePath[0xFF];
	char* endPoint;
	double startTime, endTime;
	double divStartTime, divEndTime, totalDivTime=0;
	double compStartTime, compEndTime;
	double setupTimeStart, setupTimeEnd;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_count);

	//master node section
	if(my_rank==0){
		printf("Text file path: ");
		scanf("%[^\n]s", &filePath);
		printf("%s\n", filePath);
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
		startTime = MPI_Wtime();
		//Broadcast min and max number of letter
		MPI_Bcast(&minLetter, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&maxLetter, 1, MPI_INT, 0, MPI_COMM_WORLD);
		fseek(fp, 0,SEEK_END);
		size = ftell(fp);
		rewind(fp);
		while(!feof(fp)){
   			fscanf(fp, "%c", &buffer[i]);
   			i++;
		}
		buffer[i-1]= '\0'; //Add null terminator to the end of char array
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
			setupTimeStart = MPI_Wtime();
			endPointer--;
			while(buffer[endPointer+1] != ' '){
				if(buffer[endPointer+1] == '\t')
					break;
				if(buffer[endPointer+1] == '\n')
					break;
				if(buffer[endPointer+1]!= '\0'){
					endPointer++;
				}
				else
					break;
			}
			sendbufArraySize = endPointer+1 - startPointer;
			if(sendbuf == NULL)
				sendbuf = malloc(sendbufArraySize+1); //+1 for null terminator
			else
				sendbuf = realloc(sendbuf, sendbufArraySize+1); //+1 for null terminator
			memcpy(sendbuf, buffer + startPointer, sendbufArraySize);
			setupTimeEnd = MPI_Wtime();
			printf("SETUP TIME %d: %.10f", i, setupTimeEnd-setupTimeStart);
			sendbuf[sendbufArraySize] = '\0';
			sizeSendBuffer = sendbufArraySize+1;
			MPI_Send(&sizeSendBuffer, 1, MPI_INT, i, sizeTag, MPI_COMM_WORLD); //send size of data
			divStartTime = MPI_Wtime();
			MPI_Send(sendbuf, sizeSendBuffer, MPI_CHAR, i, dataTag, MPI_COMM_WORLD);
			divEndTime = MPI_Wtime();
			printf("process %d send time : %.10f\n", i, divEndTime-divStartTime);
			totalDivTime += (divEndTime-divStartTime);
			endPointer++;
			startPointer = endPointer;
			
			if(proc_count-(i+1)!= 0){
				endPointer = endPointer+((size-endPointer)/(proc_count-(i+1)));
			}
		}
		printf("total divTime : %.10fs\n", totalDivTime);

	}
	//end of master node section
		
	//slave nodes section
	if(my_rank!=0){
		MPI_Bcast(&minLetter, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&maxLetter, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Recv(&sendbufArraySize, 1, MPI_INT, 0, sizeTag, MPI_COMM_WORLD, &status);
		compStartTime = MPI_Wtime();
		if(sendbufArraySize-1 != 0){
			recvbuf = malloc(sendbufArraySize);
			MPI_Recv(recvbuf, sendbufArraySize, MPI_CHAR, 0, dataTag, MPI_COMM_WORLD, &status);
			printf("Process %d received %d data: \n", my_rank, sendbufArraySize-1);
			i=0;
			while(recvbuf[i]==' ' || recvbuf[i] == '\t')
				i++;
			while(recvbuf[i]!= '\0'){
				if(recvbuf[i]==' ' || recvbuf[i]=='\t' || recvbuf[i]=='\n'){
					if(letterCount>=minLetter && letterCount<=maxLetter){
						localCount++;
					}
					letterCount=0;
					i++;
					continue;
				}
				else if(!ispunct(recvbuf[i])){
					if(!isdigit(recvbuf[i])){
						if(recvbuf[i]!= '\t'){
							letterCount++;
						}
					}
				}
				i++;
				if(recvbuf[i] == '\0'){
					if(letterCount>=minLetter && letterCount<=maxLetter){
						localCount++;
					}
					letterCount=0;
				}
			}
		}
		//
		else
			printf("Process %d received no data\n", my_rank);
		compEndTime = MPI_Wtime();
		printf("Process %d comp time = %.10f\n", my_rank, compEndTime-compStartTime);
		printf("Process %d has %d words\n", my_rank, localCount);
	}
	//end of slave nodes section
	
	MPI_Reduce(&localCount, &globalCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if(my_rank == 0){
		printf("Total number of words : %d\n", globalCount);
		endTime = MPI_Wtime();
		printf("Elapsed time : %.10fs\n", endTime - startTime);
	}
	MPI_Finalize();
}

