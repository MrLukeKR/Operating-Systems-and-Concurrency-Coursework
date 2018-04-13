#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>

#define NUMBER_OF_CONSUMERS 4
#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 10
#define TIME_SLICE 25

#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3

int produced, consumed =0;
int priorityCount[10];

int remainingConsumers = NUMBER_OF_CONSUMERS;

int syncval, delayproducerval, itemsval;
sem_t delay_producer;
sem_t items;
sem_t sync;

int totalStartTime = 0;
int startPointer = 0, endPointer = 0;

int jobIndex = 0;
int bufferSize = 0;

int aiJobs[BUFFER_SIZE][4];

void printJob(int iId, int iBurstTime, int iRemainingTime, int iPriority)
{
	printf("Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", iId, iBurstTime, iRemainingTime, iPriority);
}

void printJobs()
{
	int i;
	printf("JOBS: \n");
	for(i = 0; i < bufferSize; i++)
		printJob(aiJobs[i][JOB_INDEX], aiJobs[i][BURST_TIME], aiJobs[i][REMAINING_TIME], aiJobs[i][PRIORITY]);
}

void insertIntoQueue(int job[4]){
		int insertionPointer = 0;
		while(aiJobs[insertionPointer][PRIORITY]<=job[PRIORITY] && insertionPointer < bufferSize)
			insertionPointer ++;

		bufferSize++;
		int i;
		for(i = bufferSize - 1; i > insertionPointer; i--)
			memcpy(aiJobs[i], aiJobs[i-1], 4 *sizeof(int));
		memcpy(aiJobs[insertionPointer], job, 4 * sizeof(int));

		sem_post(&items);
}

void addJob()
{
		int job[4];

		job[JOB_INDEX] = jobIndex++;
		job[BURST_TIME] = (rand() % 99) + 1;
		job[REMAINING_TIME] = job[BURST_TIME];
		job[PRIORITY] = rand()%10;
		
		priorityCount[job[PRIORITY]]++;

		produced++;

		printf("Producing: JOB ID = %d, Burst Time = %d, Remaining Time = %d, Priority = %d (jobs produced = %d, jobs in buffer = %d)\n",job[JOB_INDEX],job[BURST_TIME],job[REMAINING_TIME],job[PRIORITY],produced, bufferSize+1);


		insertIntoQueue(job);
}

void printSemaphores();

void initSemaphores(){
	sem_init(&delay_producer, 0, 0);
	sem_init(&items, 0, 0);
	sem_init(&sync, 0, 1);
}

void updateSemaphoreValues(){
	sem_getvalue(&delay_producer, &delayproducerval);
	sem_getvalue(&items, &itemsval);
	sem_getvalue(&sync, &syncval);	
}

void printSemaphores(){
	updateSemaphoreValues();
	printf("sSync = %d, sDelayProducer = %d, sItems = %d\n", syncval, delayproducerval, itemsval);
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

int removeJob(){	
	int reinserted = 1;

	if((aiJobs[0][REMAINING_TIME] <= TIME_SLICE) || (priorityCount[aiJobs[0][PRIORITY]] == 1))
		reinserted = 0;
	
	int i;
	for(i = 0; i < bufferSize - 1;i++){
		memcpy(aiJobs[i], aiJobs[i+1], 4 * sizeof(int));
	}

	bufferSize--;
return reinserted;
}


int processJob(int job[4], int * startTime, int * endTime, int * currTime){
	int timeTaken = 0;

	if((job[REMAINING_TIME] <= TIME_SLICE) || (priorityCount[job[PRIORITY]] ==1)){
		timeTaken = job[REMAINING_TIME];
		*currTime += timeTaken;
		*endTime = *currTime;
	}else{
		timeTaken = TIME_SLICE;
		*currTime += timeTaken;
	}

	job[REMAINING_TIME] -=timeTaken;

	printf("JOB ID = %d, ", job[JOB_INDEX]);
	if (job[BURST_TIME] == job[REMAINING_TIME] + timeTaken){
			printf("Start Time = %d, ", *startTime);
			totalStartTime+= *startTime;
	}else
			printf("Re-start Time = %d, ", *startTime);
	if (job[REMAINING_TIME] == 0){
		printf("End Time = %d, ", *endTime);
		consumed++;
		priorityCount[aiJobs[0][PRIORITY]]--;
	}else{
		printf("Remaining Time = %d, ",job[REMAINING_TIME]);
		insertIntoQueue(job);
	}
	printf("Priority: %-1d (jobs removed = %d, jobs in buffer = %d)\n", job[PRIORITY], consumed, bufferSize);

	*startTime = *currTime;
return timeTaken;
}

void * producer(void * p){
while(produced < NUMBER_OF_JOBS){
		sem_wait(&sync);
			addJob();
		if(bufferSize == BUFFER_SIZE){
				sem_post(&sync);
				sem_wait(&delay_producer);
		}else
			sem_post(&sync);
	}
}

void * consumer(void * p){
int currentJob[4];
int id = *((int *) p);
int startTime = 0, currentTime = 0, endTime = 0;
int simulateTime = 0;
int reinserted = 0;

	while(1){
		sem_wait(&items);
		sem_wait(&sync);
			memcpy(currentJob, aiJobs[0], 4 * sizeof(int));

			reinserted = removeJob();

			printf("Thread %d removes: ", id);
			simulateTime = processJob(currentJob, &startTime, &endTime, &currentTime);

			if(bufferSize == BUFFER_SIZE - 1 && !reinserted)		//Don't post delay_producer if the job is being re-added to the queue
				sem_post(&delay_producer);

			if(NUMBER_OF_JOBS - consumed < remainingConsumers){
				remainingConsumers--;
				sem_post(&sync);
				simulateJob(simulateTime);
				break;
			}

		sem_post(&sync);
		simulateJob(simulateTime);
	}
}

int main()
{	
	initSemaphores();

	pthread_t pThread;
	pthread_t * cThread = (pthread_t *)malloc(NUMBER_OF_CONSUMERS * sizeof(pthread_t *));

	pthread_create(&pThread, NULL, producer, NULL);
	int i;
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++){
		int *id = malloc(sizeof(*id));
		*id = i+1;
		pthread_create(&cThread[i], NULL, consumer, id);
	}

	pthread_join(pThread, NULL);
	
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
		pthread_join(cThread[i], NULL);
	
	printSemaphores();
	printf("Average start time = %.1f\n", (float)totalStartTime / NUMBER_OF_JOBS*1.0);
	return 0;
}
