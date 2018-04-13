#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>

#define NUMBER_OF_CONSUMERS 4
#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 10

#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3

int produced, consumed =0;

int totalStartTime = 0;

int jobIndex = 0;
int bufferIndex = 0;

int remainingConsumers = NUMBER_OF_CONSUMERS;

int aiJobs[BUFFER_SIZE][4];

void addJob()
{
		aiJobs[bufferIndex][JOB_INDEX] = jobIndex++;
		aiJobs[bufferIndex][BURST_TIME] = (rand() % 99) + 1;
		aiJobs[bufferIndex][REMAINING_TIME] = aiJobs[bufferIndex][BURST_TIME];
		aiJobs[bufferIndex][PRIORITY] = rand()%10;

		produced++;

		printf("Producing: JOB ID = %d, Burst Time = %d, Remaining Time = %d (jobs produced = %d, jobs in buffer = %d)\n",aiJobs[bufferIndex][JOB_INDEX],aiJobs[bufferIndex][BURST_TIME],aiJobs[bufferIndex][REMAINING_TIME],produced, bufferIndex+1);
		bufferIndex++;
}

void printSemaphores();

int syncval, delayproducerval, itemsval;
sem_t delay_producer;
sem_t items;
sem_t sync;

void initSemaphores(){
	sem_init(&delay_producer, 0, 0);
	sem_init(&items, 0, 0);
	sem_init(&sync, 0, 1);
}

void updateSemaphoreValues(){
	sem_getvalue(&delay_producer, &delayproducerval);
	sem_getvalue(&items, &itemsval);
	sem_getvalue(&sync, &syncval);	

	if((delayproducerval > 1 ||  itemsval > BUFFER_SIZE ||  syncval > 1)){
		printf("ILLEGAL SEMAPHORES: \n");		
		printf("sSync = %d, sDelayProducer = %d,  sItems = %d\n", syncval, delayproducerval, itemsval);
		exit(-1);
	}
}

void printSemaphores(){
	updateSemaphoreValues();
		printf("sSync = %d, sDelayProducer = %d, sItems = %d\n", syncval, delayproducerval, itemsval);
}

void removeJob(){
	int job[4];
	memcpy(job, aiJobs[0], 4 * sizeof(int));

	bufferIndex--;
	int i;
	for(i = 1; i <= bufferIndex;i++){
		memcpy(aiJobs[i-1], aiJobs[i], 4 * sizeof(int));
	}

	consumed++;
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


void * producer(void * p){
while(produced < NUMBER_OF_JOBS){
		sem_wait(&sync);
			addJob();		
			sem_post(&items);
			if(bufferIndex == BUFFER_SIZE){
				sem_post(&sync);
				sem_wait(&delay_producer);
			}else
		sem_post(&sync);
	}
}

void * consumer(void * p){
int currentJob[4];
int id = *((int *) p);
int startTime = 0, endTime = 0;

	while(1){
		sem_wait(&items);
		sem_wait(&sync);
			printf("Thread %d removes: ", id);
			memcpy(currentJob, aiJobs[0], 4 * sizeof(int));

			removeJob();

			endTime+=currentJob[BURST_TIME];
			printf("JOB ID = %d, Burst Time = %d, Start Time = %d (jobs removed = %d, jobs in buffer = %d)\n",currentJob[JOB_INDEX],currentJob[BURST_TIME], startTime,consumed,bufferIndex);
			totalStartTime+= startTime;
			
			if(bufferIndex == BUFFER_SIZE - 1)
				sem_post(&delay_producer);
		if(NUMBER_OF_JOBS - consumed < remainingConsumers){
			remainingConsumers--;
			sem_post(&sync);
			simulateJob(currentJob[REMAINING_TIME]);
			break;
		}
		startTime+=currentJob[BURST_TIME];	

		sem_post(&sync);
		simulateJob(currentJob[REMAINING_TIME]);
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
	printf("Average start time = %.1f\n\n", (float)totalStartTime / NUMBER_OF_JOBS*1.0);
	return 0;
}

