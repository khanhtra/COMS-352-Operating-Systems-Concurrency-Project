#include <stdio.h>
#include "encrypt-module.h"
#include "stdint.h"
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

//Create a circle buffer struct
typedef struct {
	int length;
	int currentSize;
	int index;
	int readI;
	int writeI;
	char buffArray[10];
	bool empty;
} cbuff;

sem_t full;
sem_t empty;
sem_t enc;
sem_t writeD;

cbuff inputB;
cbuff outputB;

pthread_cond_t readEmpty;
pthread_cond_t writeEmpty;

pthread_mutex_t mutexBuffer;
pthread_mutex_t mutexBuffer2;


char a;

void reset_requested() {
	log_counts();
}

void reset_finished() {
}


void cBuffInit(cbuff c, int size) {
	c.length = 0;
	c.currentSize = 0;
	c.index = 0;
	c.readI = 0;
	c.writeI = 0;
	c.buffArray[size];
	c.empty = true;
}

void insertInput(char data) {
	if (inputB.writeI == inputB.length) {
		inputB.writeI = 0;
	}
	inputB.buffArray[inputB.writeI] = data;
	//printf("%c", inputB.buffArray[inputB.writeI]);
	//printf(" Index S: %d\n", inputB.writeI);
	inputB.writeI++;


}
void insertOutput(char data) {
	if (outputB.writeI == outputB.length) {
		outputB.writeI = 0;
	}
	outputB.buffArray[outputB.writeI] = data;
	//printf("%c", outputB.buffArray[outputB.writeI]);
	//printf(" Index: %d\n", outputB.writeI);
	outputB.writeI++;

}

char readIB() {
	if (inputB.readI == inputB.length) {
		inputB.readI = 0;
	}
	char result;
	result = inputB.buffArray[inputB.readI];
	//printf("%c", result);
	return result;
}

char readOB() {
	if (outputB.readI == outputB.length) {
		outputB.readI = 0;
	}
	char result;
	result = outputB.buffArray[outputB.readI];
	return result;
}
//Reader thread
void* reader_thread(void* arg)
{

	//Read char at c, input counter and encrytor must process first
	//Run until end of file, update write index of circ buffer, stop when it has no space
	while ((a = read_input()) != EOF) {
		sem_wait(&empty);
		pthread_mutex_lock(&mutexBuffer);
	//	printf("One ");
		inputB.empty = false;
		//	c->buffArray[c->writeI] = a;
			//c->writeI++;
		//printf(" Index: %d\n", inputB.writeI);
		insertInput(a);
		pthread_mutex_unlock(&mutexBuffer);
		sem_post(&full);


	}
	//sem_post(&full);
	inputB.empty = true;
	//sem_post(&full);
	printf("\n");

	//printf("Reader thread created\n");
}

//Input Counter thread
void* IC_thread(void* arg)
{
	struct timespec t;
	t.tv_nsec = 950000;
	t.tv_sec = 0;

	while (inputB.empty == false) {
		nanosleep(&t, NULL);
		sem_wait(&full);
		pthread_mutex_lock(&mutexBuffer);
	//	printf("Two");
		//if (inputB.readI == inputB.length) {
	//		inputB.readI = 0;
	//	}
		count_input(readIB());
		inputB.readI++;
		//inputB.readI++;
	//	for (int i = 0; i < inputB.length; i++) {
	//		printf("%c", inputB.buffArray[i]);
	//	}
	//printf(" Index: %d\n", inputB.readI);

		pthread_mutex_unlock(&mutexBuffer);
		sem_post(&empty);
	}
}

//Encryptor thread
void* encryptor_thread(void* arg)
{
	struct timespec t;
	t.tv_nsec = 950000;
	t.tv_sec = 0;
	char x;
	while (inputB.empty == false) {
		nanosleep(&t, NULL);
		sem_wait(&writeD);
		//sem_wait(&enc);
		pthread_mutex_lock(&mutexBuffer);
		//if (inputB.readI == inputB.length) {
	//		inputB.readI = 0;
	//	}
		x = encrypt(readIB());
		insertOutput(x);
		//inputB.readI++;
		//for (int i = 0; i < inputB.length; i++) {
	//	printf("%c", outputB.buffArray[outputB.writeI++]);
	//	}
		//printf("Index: %d\n", inputB.readI);

		pthread_mutex_unlock(&mutexBuffer);;
		sem_post(&enc);
		//sem_post(&empty);
	}
	outputB.empty = true;
}

//Writer thread
void* writer_thread(void* arg)
{
	struct timespec t;
	t.tv_nsec = 950000;
	t.tv_sec = 0;
	while (outputB.empty == false) {
		nanosleep(&t, NULL);
		//sem_wait(&full);
		pthread_mutex_lock(&mutexBuffer2);
		//if (inputB.readI == inputB.length) {
	//		inputB.readI = 0;
	//	}
		write_output(readOB());
		outputB.readI++;
		//inputB.readI++;
	//	for (int i = 0; i < inputB.length; i++) {
	//		printf("%c", inputB.buffArray[i]);
	//	}
	//	printf(" Index: %d\n", inputB.readI);
		pthread_mutex_unlock(&mutexBuffer2);
		//	sem_post(&empty);
	}
}

//Output Counter thread
void* OC_thread(void* arg)
{
	struct timespec t;
	t.tv_nsec = 950000;
	t.tv_sec = 0;

	while (outputB.empty == false) {
		nanosleep(&t, NULL);
		sem_wait(&enc);
		pthread_mutex_lock(&mutexBuffer2);
		count_output(readIB());
		outputB.readI++;
		//	for (int i = 0; i < outputB.length; i++) {
			//	printf("%c", outputB.buffArray[i]);
		//	}
		//	printf("Index: %d\n", outputB.readI);

		pthread_mutex_unlock(&mutexBuffer2);
		sem_post(&writeD);
	}
}

int main(int argc, char* argv[]) {

	int inputSize;
	int outputSize;
	char c; //Processing char
	sem_t mutex;

	if (argc < 4) {
		printf("Error! At least 3 files are needed in the form: input file name, output file name, log file name!\n");
		return 0;
	}
	if (argc > 4) {
		printf("Error! Too many file names! Re-run program!\n");
		return 0;
	}

	//Prompt for I/O buffer and store into variables
	printf("What input buffer to use? ");
	scanf("%d", &inputSize);

	//Error checking for size
	if (inputSize < 1) {
		printf("Input buffer must be at least greater than 1! Re-run program!\n");
		return 0;
	}

	printf("What output buffer to use? ");
	scanf("%d", &outputSize);

	//Error checking for size
	if (outputSize < 1) {
		printf("Output buffer must be at least greater than 1! Re-run program!\n");
		return 0;
	}

	//Initialize according to user prompt
	init(argv[1], argv[2], argv[3]);

	//Create buffers according to user prompts

	inputB.length = inputSize;
	outputB.length = outputSize;

	cBuffInit(inputB, inputSize);
	cBuffInit(outputB, outputSize);

	//Initialize concurrency variables
	sem_init(&empty, 0, 1);
	sem_init(&full, 0, 1);
	sem_init(&enc, 0, 1);
	sem_init(&writeD, 0, 1);

	pthread_mutex_init(&mutexBuffer, NULL);
	pthread_mutex_init(&mutexBuffer2, NULL);

	pthread_cond_init(&readEmpty, NULL);
	pthread_cond_init(&writeEmpty, NULL);

	//Create the other threads (reader, input counter, encryptor, output counter and writer)
	pthread_t reader;
	pthread_t inputCounter;
	pthread_t encryptor;
	pthread_t outputCounter;
	pthread_t writer;

	pthread_create(&reader, NULL, reader_thread, NULL);
	pthread_create(&inputCounter, NULL, IC_thread, NULL);
	pthread_create(&encryptor, NULL, encryptor_thread, NULL);
	pthread_create(&writer, NULL, writer_thread, NULL);
	pthread_create(&outputCounter, NULL, OC_thread, NULL);

	pthread_join(reader, NULL);
	pthread_join(inputCounter, NULL);
	pthread_join(encryptor, NULL);
	pthread_join(writer, NULL);
	pthread_join(outputCounter, NULL);


	sem_destroy(&empty);
	sem_destroy(&full);
	sem_destroy(&enc);
	sem_destroy(&writeD);

	pthread_mutex_destroy(&mutexBuffer);
	pthread_mutex_destroy(&mutexBuffer2);
	
	pthread_cond_destroy(&readEmpty);
	pthread_cond_destroy(&writeEmpty);

	printf("End of file reached.\n");
	log_counts();
}
