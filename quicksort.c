// Final example with condition variables
// Compile with: gcc -O2 -Wall -pthread cycle_buffer.c -o cbuffer

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <pthread.h>

#define N 1000000
#define CUTOFF 300
#define MESSAGES (N/CUTOFF)
#define THREADS 4




// conditional variable, signals a put operation (receiver waits on this)
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
// conditional variable, signals a get operation (sender waits on this)
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

// mutex protecting common resources
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int max_counter = 0;

typedef struct {
	int flag;
	int start;
	int tail;
	double *a;
}Info;

typedef struct {
	Info array[MESSAGES];
	int qin, qout;
	int count;
}CBUFFER;

void send_msg(Info msg);
Info recv_msg();
void *prod_con_thread(void *args);
void init_buffer();
Info make_info(int flag, int s, int t, double *a);
void inssort(double *a,int n);
int partition(double *a,int n);

CBUFFER cbuffer;

int main() {
	double *array; 
	int i, counter=0;
	array = (double *)malloc(N*sizeof(double));
	if (array==NULL) {
		printf("error in malloc\n");
		exit(1);
	}
	printf("hrhta\n");
	// fill array with random numbers
	srand(time(NULL));
	for (i=0;i<N;i++) {
		array[i] = (double)rand()/RAND_MAX;
	}

	init_buffer();
	Info info = make_info(0, 0, N, array);
	send_msg(info); // stelnoume to prwto message
	//printf("start = %d tail = %d\n",cbuffer.array[0].start, cbuffer.array[0].tail);
	pthread_t threads[THREADS]; 
	// create threads
	for(i=0;i<THREADS;i++)
		pthread_create(&threads[i],NULL,prod_con_thread,NULL);

	while(counter != N){
		info = recv_msg();
		if(info.flag != 1){ // den einai etoimo xanabaleto sto buffer
			send_msg(info);
			continue;
		}
		counter += info.tail - info.start;
		//printf("counter = %d\n", counter);
	}
	// o pinakas exei taxinomhthei prepei na teleiwsoun ta threads
	info.flag = 10;
	for(i=0;i<THREADS;i++){
		send_msg(info);
	}


	for(i=0;i<THREADS;i++)
		pthread_join(threads[i],NULL);

	//printf("array[100] = %lf", array[100]);
	// check sorting
	for (i=0;i<(N-1);i++) {
		if (array[i]>array[i+1]) {
		  	printf("%lf %lf i=%d i+1=%d Sort failed!\n", array[i], array[i+1], i, i+1);
			break;
		}
	}
	
	// destroy mutex - should be unlocked
	pthread_mutex_destroy(&mutex);

	// destroy cvs - no process should be waiting on these
	pthread_cond_destroy(&msg_out);
	pthread_cond_destroy(&msg_in);
	
	free(array);
	printf("We have used %d buffer places!\n", max_counter);
	printf("program is termitnating, Sort success!\n");
	
	return 0;
}

// producer thread function
void *prod_con_thread(void *args) {
	int pivot_pos;
	while(1){
		Info info = recv_msg();
		if(info.flag == 10)	{ // exei teleiwsei h sort
			//printf("hrtha!\n");
			pthread_exit(NULL);
		}
		else if(info.flag == 1) // ayto prepei na to dei h main
			send_msg(info);
		else if((info.tail - info.start) <= CUTOFF){ // prepei na ginei sort
			inssort(info.a+info.start, info.tail - info.start); printf("tail = %d\n", info.tail);
			info.flag = 1;
			send_msg(info);
		} 
		else{
			pivot_pos = partition((info.a+info.start), info.tail - info.start);
			Info info1 = make_info(0, info.start, info.start+pivot_pos, info.a);
			Info info2 = make_info(0, info.start+pivot_pos, info.tail, info.a);
			send_msg(info1);
			send_msg(info2);
		}
	}
}
  
// ---- send/receive functions ----

void send_msg(Info msg) {

    pthread_mutex_lock(&mutex);
    while (cbuffer.count == MESSAGES) { // NOTE: we use while instead of if! more than one thread may wake up
    				// cf. 'mesa' vs 'hoare' semantics
      pthread_cond_wait(&msg_out,&mutex);  // wait until a msg is received - NOTE: mutex MUST be locked here.
      					   // If thread is going to wait, mutex is unlocked automatically.
      					   // When we wake up, mutex will be locked by us again. 
    }
    
    // send message
    cbuffer.array[cbuffer.qin++] = msg;
    if(cbuffer.qin == MESSAGES)
    	cbuffer.qin = 0;
    cbuffer.count++;
    if(cbuffer.count > max_counter) max_counter = cbuffer.count; // gia na doume telika posew theseis xreiazomaste sto buffer
    
    // signal the receiver that something was put in buffer
    pthread_cond_signal(&msg_in);
    
    pthread_mutex_unlock(&mutex);

}


Info recv_msg() {

    // lock mutex
    pthread_mutex_lock(&mutex);
    while (cbuffer.count == 0) {	// NOTE: we use while instead of if! see above in producer code
    
      pthread_cond_wait(&msg_in,&mutex);  
    
    }
    // receive message
    Info i = cbuffer.array[cbuffer.qout++];
    if(cbuffer.qout == MESSAGES)
    	cbuffer.qout = 0;
    cbuffer.count--;
    
    // signal the sender that something was removed from buffer
    pthread_cond_signal(&msg_out);
    
    pthread_mutex_unlock(&mutex);

    return(i);
}


int partition(double *a,int n) {
	int first,last,middle;
	double t,p;
	int i,j;

	// take first, last and middle positions
	first = 0;
	middle = n/2;
	last = n-1;  

	// put median-of-3 in the middle
	if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }
	if (a[last]<a[middle]) { t = a[last]; a[last] = a[middle]; a[middle] = t; }
	if (a[middle]<a[first]) { t = a[middle]; a[middle] = a[first]; a[first] = t; }

	// partition (first and last are already in correct half)
	p = a[middle]; // pivot
	for (i=1,j=n-2;;i++,j--) {
		while (a[i]<p) i++;
		while (p<a[j]) j--;
		if (i>=j) break;

		t = a[i]; a[i] = a[j]; a[j] = t;      
	}

	// return position of pivot
	return i;
}


void inssort(double *a,int n) {
	int i,j;
	double t;

	for (i=1;i<n;i++) {
		j = i;
		while ((j>0) && (a[j-1]>a[j])) {
		  	t = a[j-1];  a[j-1] = a[j];  a[j] = t;
		  	j--;
		}
	}

}


Info make_info(int flag, int s, int t, double *a){
	Info info;
	info.flag = flag;
	info.start = s;
	info.tail = t;
	info.a = a;
	return info;
}
void init_buffer() {

	cbuffer.qin = 0;
	cbuffer.qout = 0;
	cbuffer.count = 0;

}





  



