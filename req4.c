#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>

#define NUMBER_OF_CONSUMERS 10
#define NUMBER_OF_JOBS 1000
#define BUFFER_SIZE 100

char filledItem = '*';
char emptyItem = ' ';
int produced, consumed =0;

char buffer[BUFFER_SIZE];
int bufferIndex = 0;

int remainingConsumers = NUMBER_OF_CONSUMERS;

void printSemaphores();

int syncval, delayproducerval, itemsval;
sem_t delay_producer;
sem_t items;
sem_t sync;

void initBuffer(){
int i;
for(i = 0; i < BUFFER_SIZE; i++)
	buffer[i] = emptyItem;
}


void initSemaphores(){
	sem_init(&delay_producer, 0, 0);
	sem_init(&items, 0, 0);
	sem_init(&sync, 0, 1);
}

void printBuffer(int tid){
	printf("iTID = %u, iIndex = %-4d", tid, bufferIndex);
	int i;	
	for (i = 0; i < BUFFER_SIZE;i++)
		printf("%c", buffer[i]);
	printf("\n");
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
			printBuffer(pthread_self());
			sem_post(&items);
		if(bufferIndex == BUFFER_SIZE){
				sem_post(&sync);
				sem_wait(&delay_producer);
		}else
			sem_post(&sync);
	}
}

void * consumer(void * p){
	while(1){
		sem_wait(&items);
		sem_wait(&sync);
			removeItem();
			printBuffer(pthread_self());
			if(bufferIndex == BUFFER_SIZE - 1)
				sem_post(&delay_producer);
		if(NUMBER_OF_JOBS - consumed < remainingConsumers){
			remainingConsumers--;
			sem_post(&sync);
			break;
		}
		sem_post(&sync);
	}
}

int main()
{
	initBuffer();		//Stops there from being multiple \00 characters in the output text file
	initSemaphores();

	pthread_t pThread;
	pthread_t * cThread = (pthread_t *)malloc(NUMBER_OF_CONSUMERS * sizeof(pthread_t *));

	pthread_create(&pThread, NULL, producer, NULL);
	int i;
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
		pthread_create(&cThread[i], NULL, consumer, NULL);

	pthread_join(pThread, NULL);
	
	for(i = 0; i < NUMBER_OF_CONSUMERS; i++)
		pthread_join(cThread[i], NULL);

	printSemaphores();
	return 0;
}

