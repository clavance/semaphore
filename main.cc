/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

#define INCORRECT_NUMBER_OF_PARAMETERS 1
#define NON_INTEGER_PARAMETERS 2
#define TIME_OUT 3

void *producer (void *id);
void *consumer (void *id);

typedef const int semaphore;  //'semaphore' is defined as a special integer variable (global variables to allow access by all functions). 4, 5, 6 are random integers since semaphore is an int type and should be initialised
semaphore mutex = 4; //controls access to the critical region
semaphore empty = 5; //counts the number of slots in the queue which are empty
semaphore full = 6; //counts the number of slots in the queue which are not empty

//struct to pass variables in to pthread_create function for producers
struct p_parameter_data
{
  int producer_id;
  int queue_size; //size of the circular queue
  int items_per_producer; //number of items to be produced per producer
  int *sem_ptr; //pointer to a semaphore
  int *number_of_items_ptr; //points to number of items in the queue
  int *in_ptr; //points to head of the queue
  struct Job *q_ptr; //points to the circular queue
};

//struct to pass variables in to pthread_create function for consumers
struct c_parameter_data
{
  int consumer_id;
  int queue_size;
  int *sem_ptr;
  int *number_of_items_ptr;
  int *out_ptr;
  struct Job *q_ptr;
};

//struct for each job, which has a job id and a duration
struct Job
{
  int job_id;
  int duration;
};


int main (int argc, char **argv)
{
  //there should be 5 command line parameters (argc == 5)
  const int queue_size = check_arg(argv[1]); //size of the circular queue (the buffer)
  const int items_per_producer = check_arg(argv[2]); //number of items to be produced per producer
  const int number_of_producers = check_arg(argv[3]); //number of producers
  const int number_of_consumers = check_arg(argv[4]); //number of consumers

  //checks if 5 command line parameters have been entered, else, prints error
  if (argc != 5)
  {
    cerr << "After entering the program name, please enter 4 integers as ";
    cerr << "command line parameters in the following order: ";
    cerr << "queue size, number of items produced per producer, ";
    cerr << "number of producers, and number of consumers." << endl;
    return INCORRECT_NUMBER_OF_PARAMETERS;
  }

  //checks if 4 integer parameters have been provided, else, prints error
  for (int i=1; i<5; i++)
  {
    char* a = argv[i];
    char c = 0;
    for (int j = 0; c = a[j], c; ++j)
    {
      if (!isdigit(c))
      {
          cerr << "After the program name, please enter 4 positive integers ";
          cerr << "as parameters.";
          return NON_INTEGER_PARAMETERS;
      }
    }
  }

  //creates 3 semaphores in a semaphore array with SEM_KEY
  int sem_id = sem_create(SEM_KEY, 3); //sem_create function returns an integer id
  sem_init(sem_id, mutex, 1); //initial value is set to 1 (1 = unlocked, 0 = locked)
  sem_init(sem_id, empty, queue_size); //initial value is set to n (ie. argv[1]), all slots empty
  sem_init(sem_id, full, 0); //initial value is set to 0, no slots are full

  srand(time(NULL)); //seeds random number generator

  //int *buffer = new int[queue_size]; //initialises array to be used as circular queue
  int number_of_items = 0; //counts the number of items in the circular queue
  int in = 0; //in is the head of the circular queue
  int out = 0; //out is the rear of the circular queue
  Job q[queue_size]; //initialise an array of jobs, q, of length queue_size
  Job *q_ptr = &q[0]; //pointer pointing to adress of first element of the array of jobs

  pthread_t producerid[number_of_producers]; //initialises an array of producer pthreads, of length p

  p_parameter_data *p_parameter_ptr = nullptr;
  p_parameter_ptr = new p_parameter_data[number_of_producers];

  for (int i=0; i<number_of_producers; i++)
  {
    p_parameter_ptr[i].producer_id = i + 1;
    p_parameter_ptr[i].queue_size = queue_size;
    p_parameter_ptr[i].items_per_producer = items_per_producer;
    p_parameter_ptr[i].sem_ptr = &sem_id;
    p_parameter_ptr[i].number_of_items_ptr = &number_of_items;
    p_parameter_ptr[i].in_ptr = &in;
    p_parameter_ptr[i].q_ptr = q_ptr;
    pthread_create(&producerid[i], NULL, producer, (void *) &p_parameter_ptr[i]);
  }

  pthread_t consumerid[number_of_consumers]; //initialises an array of pthreads, of length c

  c_parameter_data *c_parameter_ptr = nullptr;
  c_parameter_ptr = new c_parameter_data[number_of_consumers];

  for (int i=0; i<number_of_consumers; i++)
  {
    c_parameter_ptr[i].consumer_id = i + 1;
    c_parameter_ptr[i].queue_size = queue_size;
    c_parameter_ptr[i].sem_ptr = &sem_id;
    c_parameter_ptr[i].number_of_items_ptr = &number_of_items;
    c_parameter_ptr[i].out_ptr = &out;
    c_parameter_ptr[i].q_ptr = q_ptr;
    pthread_create(&consumerid[i], NULL, consumer, (void *) &c_parameter_ptr[i]);
  }

  for (int i=0; i<number_of_producers; i++)
  {
    pthread_join (producerid[i], NULL);
  }

  for (int i=0; i<number_of_consumers; i++)
  {
    pthread_join (consumerid[i], NULL);
  }

  sem_close(sem_id); //destroy the semaphore array when finished

  return 0;
}


//producer function
void *producer (void *p_parameter)
{
  //pass struct which contains the information needed into producer function
  struct p_parameter_data *producer_data;
  producer_data = (struct p_parameter_data *) p_parameter; //cast to correct data type for producer argument

  int producer_id = producer_data->producer_id;
  int queue_size = producer_data->queue_size;
  int items_per_producer = producer_data->items_per_producer;
  int *sem_ptr = producer_data->sem_ptr;
  int &sem_id = *sem_ptr;
  int *number_of_items_ptr = producer_data->number_of_items_ptr;
  int &number_of_items = *number_of_items_ptr;
  int *in_ptr = producer_data->in_ptr;
  int &in = *in_ptr;
  Job *q = producer_data->q_ptr;
  Job j;

  //sem_timedwait has an argument which points this struct
  struct timespec ts;
  ts.tv_sec = 20; //sets time limit in timeout to 20s
  int s = sem_timedwait(sem_id, empty, &ts); //s is the integer value returned by sem_timedwait

  int loop = items_per_producer;

  for (int i=0; i<loop; i++) //loop items_per_producer times
  {
    if (number_of_items != 0 && number_of_items < queue_size)
    {
      sleep(rand() % 5 + 1); //if there is at least one item, sleep for 1s - 5s before producing another item
    }

    while (s == -1 && errno == EINTR) //if sem_timedwait returns -1 and the error returned is EINTR, the call was interrupted by a signal not a timeout, so retry
      {
        continue;
      }

    if (s == -1 && errno == ETIMEDOUT) //if the semaphore could not be locked before 20s expired, error returned will be ETIMEDOUT
    {
      pthread_exit(0);
      return ((void *) TIME_OUT); //as producer is a void function, it cannot return an int
    }

    int duration = rand() % 10 + 1; //generate a random duration from 1 to 10s
    j.job_id = in + 1; //the first job is Job ID: 1, while array index starts at 0
    j.duration = duration;

    if (number_of_items < queue_size) //if the circular queue is not full
    {
      sem_wait(sem_id, empty); //-1 from number of empty slots remaining
      sem_wait(sem_id, mutex); //lock to prevent consumer from entering critical section at same time

      q[in] = j; //job is put in the first element of the array
      number_of_items += 1; //the number of items in the array is increased by 1
      in = (in + 1) % queue_size; //move up the head of the queue by 1. since it is a circular queue, % queue_size moves it back to the front of the queue if it reaches the end

      cout << "Producer(" << producer_id << "): ";
      cout << "Job id " << j.job_id << " ";
      cout << "duration " << duration;
      cout << endl;

      items_per_producer--; //reduce number of items left for this producer to produce

      sem_signal(sem_id, mutex); //unlock
      sem_signal(sem_id, full); //+1 item to queue
    }
  }

  if (items_per_producer == 0) //if there are no more items for this producer to produce
  {
    cout << "Producer(" << producer_id << "): ";
    cout << "No more jobs to generate.";
    cout << endl;
    pthread_exit(0);
  }

  pthread_exit(0);
}


//consumer function
void *consumer (void *c_parameter)
{
  struct c_parameter_data *consumer_data;
  consumer_data = (struct c_parameter_data *) c_parameter;

  int consumer_id = consumer_data->consumer_id;
  int queue_size = consumer_data->queue_size;
  int *sem_ptr = consumer_data->sem_ptr;
  int &sem_id = *sem_ptr;
  int *number_of_items_ptr = consumer_data->number_of_items_ptr;
  int &number_of_items = *number_of_items_ptr;
  int *out_ptr = consumer_data->out_ptr;
  int &out = *out_ptr;
  Job *q = consumer_data->q_ptr;
  Job j;

  struct timespec ts;
  ts.tv_sec = 20;
  int s = sem_timedwait(sem_id, empty, &ts);

  while (s == -1 && errno == EINTR)
    {
      continue;
    }

  while (true)
  {
    if (s == -1 && errno == ETIMEDOUT)
    {
      pthread_exit(0);
      return ((void *) TIME_OUT);
    }

    if (number_of_items > 0) //if there are any jobs to consume in the queue
    {
      sem_wait(sem_id, full); //-1 item
      sem_wait(sem_id, mutex); // lock

      j = q[out];
      number_of_items--; //consume an item from the queue
      out = (out + 1) % queue_size; //move up the rear of the queue by 1

      cout << "Consumer(" << consumer_id <<"): ";
      cout << "Job id " << j.job_id << " ";
      cout << "executing sleep duration " << j.duration;
      cout << endl;

      sem_signal(sem_id, mutex); //unlock
      sem_signal(sem_id, empty); //+1 empty space in the queue

      sleep(j.duration); //sleep for duration of job

      cout << "Consumer(" << consumer_id <<"): ";
      cout << "Job id " << j.job_id << " completed";
      cout << endl;
    }

    if (number_of_items == 0)
    {
      cout << "Consumer(" << consumer_id << "): ";
      cout << "No more jobs left.";
      cout << endl;
      pthread_exit(0);
    }
  }

  pthread_exit (0);
}
