#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define NUMBER_OF_JOBS 1000
#define TIME_SLICE 25
#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3

int aiJobs[NUMBER_OF_JOBS][4];
int responseTime = 0, avgTurnaround = 0;
int startTime = 0, currentTime = 0, endTime = 0;
int remainingJobs = NUMBER_OF_JOBS;

void generateJobs()
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS;i++)
	{
		aiJobs[i][JOB_INDEX] = i;
		aiJobs[i][BURST_TIME] = (rand() % 99)+ 1;
		aiJobs[i][REMAINING_TIME] = aiJobs[i][BURST_TIME];
		aiJobs[i][PRIORITY] = rand()%10;
	}
}

void printJob(int iId, int iBurstTime, int iRemainingTime, int iPriority)
{
	printf("Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", iId, iBurstTime, iRemainingTime, iPriority);
}

void printJobs()
{
	int i;
	printf("JOBS: \n");
	for(i = 0; i < NUMBER_OF_JOBS; i++)
		printJob(aiJobs[i][JOB_INDEX], aiJobs[i][BURST_TIME], aiJobs[i][REMAINING_TIME], aiJobs[i][PRIORITY]);
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
	int iSeconds = end.tv_sec - start.tv_sec;
	int iUSeconds = end.tv_usec - start.tv_usec;
 	long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
	return mtime;
}

void simulateJob(int iTime)
{
	long int iDifference = 0;
	struct timeval startTime, currentTime;
	gettimeofday(&startTime, NULL);
	do
	{
		gettimeofday(&currentTime, NULL);
		iDifference = getDifferenceInMilliSeconds(startTime, currentTime);
	} while(iDifference < iTime);
}

void sortJobsByPriority(){
	int tempJob[4];
	printf("\nSorting by priority\n");
	int i, j;
	for (i = 0; i < NUMBER_OF_JOBS -1; i ++)
		for (j = 0;j<NUMBER_OF_JOBS -1; j ++){
			if(aiJobs[j][PRIORITY] > aiJobs[j+1][PRIORITY]){
				memcpy(tempJob, aiJobs[j], sizeof(int)*4);
				memcpy(aiJobs[j], aiJobs[j+1], sizeof(int)*4);
				memcpy(aiJobs[j+1], tempJob, sizeof(int)*4);
			}
		}
}

int doJob(int index, int doEntireJob){
	int timeTaken = 0;

	if(aiJobs[index][REMAINING_TIME] <= TIME_SLICE || doEntireJob){
		timeTaken = aiJobs[index][REMAINING_TIME];
		currentTime += timeTaken;
		endTime = currentTime;	
		remainingJobs -= 1;
	}else{
		timeTaken = TIME_SLICE;
		currentTime += timeTaken;
	}
	
	simulateJob(timeTaken);
	aiJobs[index][REMAINING_TIME] -= timeTaken;

	printf("JOB ID = %d, ", aiJobs[index][JOB_INDEX]);
	if (aiJobs[index][BURST_TIME] == aiJobs[index][REMAINING_TIME] + timeTaken){
			printf("Start Time = %d, ",startTime);
			responseTime += startTime;
	}else
			printf("Re-start Time = %d, ",startTime);
	if (aiJobs[index][REMAINING_TIME] == 0){
		printf("End Time = %d, ", endTime);
		avgTurnaround +=endTime;
	}else
		printf("Remaining Time = %d, ",aiJobs[index][REMAINING_TIME]);
	printf("Priority: %-1d\n", aiJobs[index][PRIORITY]);

		startTime = currentTime;

	return aiJobs[index][REMAINING_TIME];
}

void reAddToQueue(int start, int end){
	int job[4];

	memcpy(job, aiJobs[start], sizeof(int)*4);
	int i;
	for (i = start; i < end; i++)
		memcpy(aiJobs[i], aiJobs[i+1],sizeof(int)*4);

	memcpy(aiJobs[end],job,sizeof(int)*4);	
}

void roundRobin(){
	int startPointer = 0, endPointer = 0;
	int currentPriority = aiJobs[startPointer][PRIORITY];
	int priorityCount;

	printf("\nROUND ROBIN; time Slice %d,\n", TIME_SLICE);
	
	while(remainingJobs > 0){
		while(aiJobs[endPointer+1][PRIORITY] == currentPriority)
			endPointer++;

		while(startPointer <= endPointer){
			if(aiJobs[startPointer][REMAINING_TIME] > 0)
				if(startPointer == endPointer){
					doJob(startPointer, 1);
					startPointer++;
				}else if(doJob(startPointer, 0) > 0)
					reAddToQueue(startPointer, endPointer);
				else
					startPointer++;
		}

		if(currentPriority < 9){
			currentPriority = aiJobs[startPointer][PRIORITY];
			endPointer = startPointer;
		}
	}
	printf("Average Response Time: %.1f\nAverage Turnaround Time: %.1f\n\n", (float)responseTime / NUMBER_OF_JOBS*1.0 , (float)avgTurnaround / NUMBER_OF_JOBS *1.0);
}

int main()
{
	generateJobs();
	printJobs();
	sortJobsByPriority();
	printJobs();
	roundRobin();

	return 0;
}

