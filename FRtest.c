#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char* argv[]){
	int my_rank, proc_count;
	FILE *fp; //file pointer
	char buffer[0xFFFFF];
	int i=0, ii, size, startPointer, endPointer, sendbufArraySize, sizeSendBuffer;
	char *sendbuf = NULL, *recvbuf;
	int sizeTag=1, dataTag=2;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_count);

	//master node section
	if(my_rank==0){	
		fp = fopen("text.txt", "r");
		fseek(fp, 0,SEEK_END);
		size = ftell(fp);
		rewind(fp);
		while(!feof(fp)){
   			fscanf(fp, "%c", &buffer[i]);
   			i++;
		}
		buffer[i-1]= '\0'; //Add null terminator to the end of char array
		printf("%s\n", buffer); //debug purpose only
		fclose(fp);

		//data set splitting with division
		/*
			array of 10, 3 processes
			1. first process get 10/3 data
			2. endPointer = 10/3 = 3
			3. check if the data contains detached word 
			4. move endPointer by +1 until the data contains no detached word
			5. e.g: endPointer = 10/3 = 3, endpointer++, endPointer ++, endPointer = 5
			6. firstProcess gets 5 data
			7. array of 10 - 5 = 5
			8. process 2 gets 5/2 = 2
			9. repeats
		*/
		startPointer = 0;
		endPointer = size/(proc_count-1);
		printf("%d\n\n", size); //debug purpose
		for(i=1; i<proc_count; i++){
			printf("%d\n", startPointer);
			endPointer--;
			while(buffer[endPointer+1] != ' '){
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
				printf("sendbuf size = %d\n", sendbufArraySize);
			for(ii=0; ii<sendbufArraySize; ii++){
				sendbuf[ii] = buffer[startPointer+ii];
			}
			sendbuf[sendbufArraySize] = '\0';
			printf("%s\n", sendbuf);
			sizeSendBuffer = sendbufArraySize+1;
			MPI_Send(&sizeSendBuffer, 1, MPI_INT, i, sizeTag, MPI_COMM_WORLD); //send size of data



			printf("%d\n", endPointer);
			endPointer++;
			startPointer = endPointer;
			if(size - endPointer == 0){ //check if there are still data remaining
				//send 0 to remaining processes
				continue;
			}
			if(proc_count-(i+1)!= 0){
				printf("size - endPointer = %d\n", size-endPointer);
				printf("proc count = %d\n\n", proc_count-(i+1));
				endPointer = endPointer+((size-endPointer)/(proc_count-(i+1)));
			}
		}

	}
	//end of master node section

	//slave nodes section
	if(my_rank!=0){
		MPI_Recv(&sendbufArraySize, 1, MPI_INT, 0, sizeTag, MPI_COMM_WORLD, &status);
		printf("Process %d received %d data\n", my_rank, sendbufArraySize-1);
	}
	MPI_Finalize();
}

