#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>

#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100

char filledItem = '*';
char emptyItem = ' ';
int produced, consumed =0;

char buffer[BUFFER_SIZE];
int bufferIndex = 0;

void printSemaphores();

int syncval, delayproducerval, delayconsumerval;
sem_t delay_producer;
sem_t delay_consumer;
sem_t sync;

void initBuffer(){
int i;
for(i = 0; i < BUFFER_SIZE; i++)
	buffer[i] = emptyItem;
}

void initSemaphores(){
	sem_init(&delay_consumer,0,0);
	sem_init(&delay_producer, 0, 0);
	sem_init(&sync, 0, 1);
}

void printBuffer(){
	printf("iIndex = %-4d", bufferIndex);
	int i;
	for (i = 0; i < BUFFER_SIZE;i++)
		printf("%c", buffer[i]);
	printf("\n");
}

void updateSemaphoreValues(){
	sem_getvalue(&delay_consumer, &delayconsumerval);
	sem_getvalue(&delay_producer, &delayproducerval);
	sem_getvalue(&sync, &syncval);	
}

void printSemaphores(){
	updateSemaphoreValues();
	printf("sSync = %d, sDelayProducer = %d, sDelayConsumer = %d\n", syncval, delayproducerval, delayconsumerval);
}

void addItem(){
	buffer[bufferIndex++] = filledItem;
	produced++;
}

void removeItem(){
	int i;
	for (i = 0; i < bufferIndex -1; i ++)
		buffer[i] = buffer[i+1];
	bufferIndex--;
	buffer[bufferIndex] = emptyItem;
	consumed++;
}

void * producer(void * p){
while(produced < NUMBER_OF_JOBS){
		sem_wait(&sync);
			addItem();
			printBuffer();
			if(bufferIndex == 1)
				sem_post(&delay_consumer);

			if(bufferIndex == BUFFER_SIZE){
				sem_post(&sync);
				sem_wait(&delay_producer);
			}else
				sem_post(&sync);
	}
}

void * consumer(void * p){
	sem_wait(&delay_consumer);
	while(1){
		sem_wait(&sync);
			removeItem();
			printBuffer();
			if(bufferIndex == BUFFER_SIZE - 1)
				sem_post(&delay_producer);

			if(bufferIndex == 0){
				if(consumed == NUMBER_OF_JOBS){
					sem_post(&sync);
					break;
				}			
				sem_post(&sync);
				sem_wait(&delay_consumer);
			}else
				sem_post(&sync);
	}
}

int main()
{
	initBuffer();		//Stops there from being multiple \00 characters in the output text file
	initSemaphores();

	pthread_t pThread, cThread;

	pthread_create(&cThread, NULL, consumer, NULL);
	pthread_create(&pThread, NULL, producer, NULL);

	pthread_join(pThread, NULL);
	pthread_join(cThread, NULL);

	printSemaphores();
	return 0;
}

