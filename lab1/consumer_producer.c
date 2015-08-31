/*  main.c  - main */
#include <xinu.h>
#include <stdio.h>
#define BUFFER_SIZE 100
void insertItem(sid32, sid32, sid32), deleteItem(sid32, sid32, sid32), addItem(int32);
int32 removeItem();
/* We must check to see if the queue is either full or empty.
 * The producer must go to to sleep when the queue is full, while the consumer
 * must go to sleep when the queue is empty. In order to accomplish this, the consumer
 * needs to notify the producer when the final queue element is dequeued. If the queue fills up
 * after enqueueing an element, then the producer must notify the consumer. Doing this will ensure
 * optimal work.
 *
 * To prevent simultaneous interference, a mutex semaphore is used. By making it wait
 * after one semaphore and signal before the other, no simultaneity can occur.
 *
 * Deadlock may also occur if both the consumer and the producer need to be awakened at the same time.
 * By using semaphores, mutual exclusion can be achieved, which will quell this situation.
 *
 */
int32 head = 0; //head element of array
int32 tail = 0; //tail element of array
int32 myQueue[BUFFER_SIZE]; //the queue, which is actually an
int32 length = 0;
int32 i = 0; //counter being used for what goes into the queue
void insertItem(sid32 consumed, sid32 produced, sid32 mutex)
{
	while(1)
	{
		wait(produced); //decrement produced semaphore
		wait(mutex); //decrement mutex semaphore
		kprintf("Being enqueued is %d\n", i); //print what is being inserted
		addItem(i++); //adds item into queue
		signal(mutex); //increment mutex semaphore
		signal(consumed); //increment consumed semaphore
	}
}
void addItem(int32 n)
{
	if(length == 0) //head and tail are both zero, so tail does not increment
	{
		myQueue[tail] = n;
		length++;
	}
	else if(length < BUFFER_SIZE && tail != (BUFFER_SIZE - 1))
	{
		tail++; //queue isn't full, so move tail forward one
		myQueue[tail] = n;
		length++;
	}
	else if(length < BUFFER_SIZE)
	{
		tail = 0; //queue is full, which means we reset tail to zero
		myQueue[tail] = n;
		length++;
	}
	else
	{

	}
}
void deleteItem(sid32 consumed, sid32 produced, sid32 mutex)
{
	while(1)
	{
		wait(consumed); //decrement consumed semaphore
		wait(mutex); //decrement mutex semaphore
		int32 deletedItem = removeItem(); //the removed item
		kprintf("Being dequeued is %d\n", deletedItem); //dequeued and printed
		signal(mutex); //increment mutex semaphore
		signal(produced); //increment produced semaphore
	}
}
int32 removeItem()
{
	int32 returnValue = myQueue[head];
	myQueue[head] = NULL;
	if(head == tail && length == 1) //only one value, which means the tail and the head are at the same position, so don't do a thing
	{
		length--;
	}
	else if(head != (BUFFER_SIZE - 1))
	{
		head++; //not at the end of the queue, so increment head
	    length--;
	}
	else
	{
		head = 0; //at the end, so reset to zero
	    length--;
	}
	return returnValue;
}
int main(int argc, char **argv)
{
	//uint32 retval;
	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	sid32 produced, consumed, mutex;//producer, consumer, and mutex semaphores
	consumed = semcreate(0); //consumer semaphore, initially full
	produced = semcreate(BUFFER_SIZE); //producer semaphore, initially empty
	mutex = semcreate(1); //mutex semaphore, initially 1
	resume(create(insertItem, 1024, 20, "insertItem", 3, consumed, produced, mutex)); //enqueues and prints the tail of the queue
	resume(create(deleteItem, 1024, 20, "deleteItem", 3, consumed, produced, mutex)); //dequeues and prints the removed queue element
	/* Wait for shell to exit and recreate it */

//	recvclr();
//	while (TRUE) {
//		retval = receive();
//		kprintf("\n\n\rMain process recreating shell\n\n\r");
//		resume(create(shell, 4096, 1, "shell", 1, CONSOLE));
//	}
//	while (1);

	return OK;
}

