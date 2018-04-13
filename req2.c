#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>

#define NUMBER_OF_JOBS 1000

int items = 0;
int consumed = 0, produced = 0;

int syncval, delayval;
sem_t delay_consumer;
sem_t sync;

void initSemaphores(){
	sem_init(&delay_consumer, 0, 0);
	sem_init(&sync, 0, 1);
}

void printItems(){
		printf("iIndex = %d\n", items);
}

void printSemaphores(){
	sem_getvalue(&sync, &syncval);
	sem_getvalue(&delay_consumer, &delayval);	

	printf("sSync = %d, sDelayConsumer = %d\n", syncval, delayval);
}

void * producer(void * p){
	while(produced < NUMBER_OF_JOBS){
		sem_wait(&sync);
		items++;
		produced++;
		printItems();
		if(items == 1)
			sem_post(&delay_consumer);
		sem_post(&sync);
	}
}

void * consumer(void * p){
	sem_wait(&delay_consumer);
	while(1){
		sem_wait(&sync);
		items--;
		consumed++;
		printItems();
		if(items == 0){
			sem_post(&sync);
			if(consumed == NUMBER_OF_JOBS)
				break;
			sem_wait(&delay_consumer);
		}else
			sem_post(&sync);
	}
}

int main()
{
	initSemaphores();

	pthread_t producer_thread, consumer_thread;

	pthread_create(&consumer_thread, NULL, consumer, NULL);
	pthread_create(&producer_thread, NULL, producer, NULL);

	pthread_join(consumer_thread, NULL);
	pthread_join(producer_thread, NULL);

	printSemaphores();
	return 0;
}

