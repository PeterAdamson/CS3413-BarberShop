//Author Peter Adamson
//command line arguments accepted:
//					-n: the program expects an integer following this argument specifying the number of clients
//					-stop: the program expects an integer following this argument specifying how long to run
//					-min: the program expects an integer following this argument specifying the minimum random number to be generated
//					-max: the program expects an integer following this argument specifiying the maximum random number to be generated
//					-chairs: the program expects an integer following this argument specifying the number of chairs in the waiting room

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
//#include <unistd.h>

//define the node structure for the queue
typedef struct qNode Node;
struct qNode
{
	int clientNumber;
	int cutTime;
	Node *next;	
};

//global variable declarations
int n;
int stop;
int min;
int max;
int chairs;
int isSleeping; //0 if awake, 1 if sleeping
int clientCount;	//counter to assign client numbers
int clientBeingCut;
int cutTime;
int count;
Node **waiting;	//number of waiting slots available
pthread_mutex_t lock1;

//function declarations
void *Client();
void *Barber();
int randInt(int min, int max);

int main(int argc, char **argv)
{
	if(argc != 11)	//incorrect number of arguments have been specified
	{
		printf("incorrect number of arguments supplied\n");
		return 1;
	}

	//iterate through the arguments
	int i;
	for(i = 1; i < argc - 1; i = i + 2)
	{
		int j = i + 1;
		if(strcmp(argv[i],"-n") == 0)	//we have the number of clients
		{
			n = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-stop") == 0)	//we have the timeout time
		{
			stop = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-min") == 0)	//we have the minimum random number to generate
		{
			min = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-max") == 0)	//we have the maximum random number to generate
		{
			max = atoi(argv[j]);
		}
		else if(strcmp(argv[i],"-chairs") == 0)	//we have the number of chairs in the waiting room
		{
			chairs = atoi(argv[j]);
		}
		else					//we have an invalid argument
		{
			printf("one or more arguments specified incorrectly\n");
			return 1;
		}
	}

	waiting = malloc(chairs * sizeof(waiting[0]));
	
	//set up our end time condition
	time_t endAt;
	time_t start = time(NULL);
	time_t end = stop;
	endAt = start + end;

	//variable initialization
	count = 0;
	clientCount = 1;
	isSleeping = 1;
	cutTime = 0;
	/*for(i = 0; i < chairs; i++)
	{
		waiting[i] = NULL;
	}
	waiting = NULL;*/

	//declare the barber and clients
	pthread_t barberThread;
	pthread_t clientThread[n];

	//create the barber and clients
	pthread_create(&barberThread, NULL, Barber, NULL);
	for(i = 0; i < n; i++)
	{
		pthread_create(&clientThread[i], NULL, Client, NULL);
	}

	//loop the main thread until timeout has been reached
	while(1)
	{
		if(start < endAt)	//we have not reached the timeout
		{
			sleep(1);
			start = time(NULL);
		}
		else			//we have reached the timeout
		{
			pthread_cancel(barberThread);	//cancel the barber
			for(i = 0; i < n; i++)
			{
				pthread_cancel(clientThread[i]);	//cancel the clients
			}
			break;
		}
	}

	//free up assigned waiting memory
	for(i = 0; i < chairs; i++)
	{
		free(waiting);
		waiting = NULL;
	}
}

void *Barber()
{
	while(1)	//continuous loop
	{
		while(isSleeping == 1)	//barber is sleeping
		{
			printf("The barber is sleeping.\n");
			sleep(1);
		}
		while(isSleeping == 0)	//barber is awake
		{
				pthread_mutex_lock(&lock1);
				printf("client %d is getting their hair cut for %d seconds.\n",clientBeingCut,cutTime);
				isSleeping = 0;
				count = count - 1;
				sleep(cutTime);	
				if(count == 0)	//there are no clients waiting
				{
					cutTime = 0;
					isSleeping = 1;
					pthread_mutex_unlock(&lock1);
					break;
				}
				else	//there is at least one client waiting
				{
					Node *client = waiting[0];
					clientBeingCut = client->clientNumber;
					cutTime = client->cutTime;

					//move clients up the waiting list
					int i;
					int j;
					for(i = 0; i < count - 1; i++)
					{
						for(j = i + 1; j > count; j++);
							waiting[i] = waiting[j];
							waiting[j] = NULL;
					}
					isSleeping = 0;
					pthread_mutex_unlock(&lock1);
				}
		}
	}	
}

void *Client()
{
	pthread_mutex_lock(&lock1);
	int clientNumber = clientCount;
	clientCount = clientCount + 1;
	pthread_mutex_unlock(&lock1);
	int arrivalTime;
	int clientCutTime;
	pthread_mutex_lock(&lock1);
	pthread_mutex_unlock(&lock1);
	Node *client = NULL;
	client = (Node*)malloc(sizeof(Node));
	client->clientNumber = clientNumber;
	while(1)	//continuous loop
	{
		arrivalTime = randInt(min, max);
		clientCutTime = randInt(min, max);
		client->cutTime = clientCutTime;
		sleep(arrivalTime);
		pthread_mutex_lock(&lock1);
		if(count <= chairs)	//the barber is either available, or waiting slots are available
		{
			if(count == 0)	//No clients in line, barber is available
			{
				isSleeping = 0;
				count = count + 1;
				clientBeingCut = clientNumber;
				cutTime = clientCutTime;
				printf("client %d went straight to the barber\n",clientBeingCut);
				pthread_mutex_unlock(&lock1);
			}
			else	//clients in line, or one with the barber
			{
				printf("client %d is waiting.\n",clientNumber);
				waiting[count - 1] = client;
				count = count + 1;	
				pthread_mutex_unlock(&lock1);
			}
		}
		else	//barber is not available and no waiting slots are available
		{
			pthread_mutex_unlock(&lock1);
			printf("client %d left.\n",clientNumber);
		}
	}
}

//generates a random integer between min and max (inclusively)
int randInt(int min, int max)
{
	return rand() % (max + 1 - min) + min;
}
