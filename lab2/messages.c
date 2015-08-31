/*  main.c  - main */
#include <xinu.h>
#include <stdio.h>
void enqueueElement(struct procent *prptr, umsg32 message);
umsg32 dequeueElement(struct procent *prptr);
syscall sendMsg(pid32 pid, umsg32 msg);
umsg32 receiveMsg(void);
uint32 sendMsgs(pid32 pid, umsg32* msgs, uint32 msg_count);
syscall receiveMsgs(umsg32* msgs, uint32 msg_count);
uint32 sendnMsg(uint32 pid_count, pid32* pids, umsg32 msg);
void testMethod(void);
/*
 * I have changed the process table by adding a message queue,
 * a head and tail element, and a length-tracker for each
 * process. By doing this, I am able to keep track of the messages
 * that each process has, via sending and receiving.
 *
 *  Definition of the process table (multiple of 32 bits)
 *  Original process table (cannot be used)
 * struct procent
 * {		entry in the process table
 * 	uint16	prstate;	process state: PR_CURR, etc.
 *	pri16	prprio;		process priority
 *	char	*prstkptr;	saved stack pointer
 *	char	*prstkbase;	base of run time stack
 *	uint32	prstklen;	stack length in bytes
 *	char	prname[PNMLEN];	process name
 *	uint32	prsem;		semaphore on which process waits
 *	pid32	prparent;	id of the creating process
 *	umsg32	prmsg;		message sent to this process
 *	bool8	prhasmsg;	nonzero iff msg is valid
 *	int16	prdesc[NDESC];	device descriptors for process
 * };
 * New process table, that contains a queue to hold the
 * messages sent to a process
 *
 * #define MAXMESSAGES 10  represents max number of messages in queue
 * struct procent {		 entry in the process table
 *	uint16	prstate;	 process state: PR_CURR, etc.
 *	pri16	prprio;		 process priority
 *	char	*prstkptr;	 saved stack pointer
 *	char	*prstkbase;	 base of run time stack
 *	uint32	prstklen;	 stack length in bytes
 *	char	prname[PNMLEN];	 process name
 *	uint32	prsem;		 semaphore on which process waits
 *	pid32	prparent;	 id of the creating process
 *	umsg32	prmsg;		 message sent to this process
 *	umsg32  messagequeue[MAXMESSAGES];    queue to represent sent messages
 *	int32   queuehead;    represents head of queue
 *	int32   queuetail;    represents tail of queue
 *	int32   queuelength;   represents length of queueu
 *	bool8	prhasmsg;	 nonzero iff msg is valid
 *	int16	prdesc[NDESC];	 device descriptors for process
 * };
 *
 * My main method runs test code. It works by sending ten messages
 * to the queue, receiving them all one-by-one, sending them as a batch,
 * receiving them as a batch, and then sending a message to multiple processes.
 */
void enqueueElement (struct procent *prptr, umsg32 message)
{
	if(prptr->queuelength == 0) //head and tail are both zero, so tail does not increment
	{
		prptr->messagequeue[prptr->queuetail] = message;
		prptr->queuelength++;
	}
	else if(prptr->queuelength < MAXMESSAGES && prptr->queuetail != (MAXMESSAGES - 1))
	{
		prptr->queuetail++; //queue isn't full, so move tail forward one
		prptr->messagequeue[prptr->queuetail] = message;
		prptr->queuelength++;
	}
	else if(prptr->queuelength < MAXMESSAGES)
	{
		prptr->queuetail = 0; //queue is full, which means we reset tail to zero
		prptr->messagequeue[prptr->queuetail] = message;
		prptr->queuelength++;
	}
}
umsg32 dequeueElement(struct procent *prptr)
{
	umsg32 returnValue = prptr->messagequeue[prptr->queuehead]; //value at head of queue
	prptr->messagequeue[prptr->queuehead] = NULL; //set it to null
	if((prptr->queuehead == prptr->queuetail) && prptr->queuelength == 1) //only one value, which means the tail and the head are at the same position, so don't do a thing
	{
		prptr->queuelength--;
	}
	else if(prptr->queuehead != (MAXMESSAGES - 1))
	{
		prptr->queuehead++; //not at the end of the queue, so increment head
		prptr->queuelength--;
	}
	else
	{
		prptr->queuehead = 0; //at the end, so reset to zero
		prptr->queuelength--;
	}
	return returnValue;
}
syscall sendMsg(pid32 pid, umsg32 msg)
{
	    intmask	mask;			/* saved interrupt mask		*/
		struct	procent *prptr;	/* ptr to process' table entry	*/
		mask = disable();
		if (isbadpid(pid)) {
			restore(mask);
			return SYSERR;
		}
		prptr = &proctab[pid];
		if ((prptr->prstate == PR_FREE) || (prptr->queuelength == MAXMESSAGES)) //replaced original code to deal with process table
		{
			restore(mask);
			kprintf("Error! Too many elements.\n"); //queue is full
			return SYSERR;
		}
		enqueueElement(prptr,msg); //enqueues the element
		/* If recipient waiting or in timed-wait make it ready */
		if (prptr->prstate == PR_RECV) {
			ready(pid, RESCHED_YES);
		} else if (prptr->prstate == PR_RECTIM) {
			unsleep(pid);
			ready(pid, RESCHED_YES);
		}
		restore(mask);		/* restore interrupts */
		kprintf("Process %d is being sent the message %s\n", pid, msg);
		kprintf("One message sent\n");
		return OK;
}
umsg32 receiveMsg(void)
{
		intmask	mask;			/* saved interrupt mask		*/
		struct	procent *prptr;		/* ptr to process' table entry	*/
		umsg32	msg;			/* message to return		*/
		mask = disable();
		prptr = &proctab[currpid]; //gets current process
		if (prptr->queuelength == 0) { //empty queue
			prptr->prstate = PR_RECV;
			resched();		/* block until message arrives	*/
		}
		msg = dequeueElement(prptr); //dequeue element
		restore(mask);
		kprintf("Message %s received by process %d\n", msg, currpid);
		return msg;
}
uint32 sendMsgs(pid32 pid, umsg32* msgs, uint32 msg_count)
{
	if(msg_count > 10) //will be more than the queue can handle
	{
		kprintf("Error. Attempted to send more than 10 messages.\n");
		return SYSERR;
	}
	else
	{
		int32 i = 0;
		while(i < msg_count)
		{
			if(sendMsg(pid, *(msgs + i)) == SYSERR) //attempt to send messages one at a time
			{
				kprintf("Sending message %s failed. Process ending.\n", *(msgs + i)); //queue is full, end process
				return SYSERR;
			}
			else
			{
				i++; //queue still has room, so continue
			}
		}
		kprintf("Sent %d messages to process %d\n", msg_count, pid);
		return msg_count;
	}
}
syscall receiveMsgs(umsg32* msgs, uint32 msg_count)
{
	if(msg_count > 10) //more than the queue can hold
	{
		kprintf("Error. Trying to receive more than 10 messages.\n");
		return SYSERR;
	}
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	prptr = &proctab[currpid];
	if (prptr->queuelength < msg_count) { //not all the messages have been sent
		prptr->prstate = PR_RECV;
		resched();		/* block until all messages have been sent */
	}
	int32 i = 0;
	while(i < msg_count)
	{
		umsg32 msg = receiveMsg(); //receives message
		i++;
	}
	restore(mask);
	kprintf("%d messages received by process %d\n", msg_count, currpid);
	return OK;
}
uint32 sendnMsg (uint32 pid_count, pid32* pids, umsg32 msg)
{
	if(pid_count > 3) //too many processes being sent messages
	{
		kprintf("Error. Attempted to send message to more than 3 processes.\n");
		return SYSERR;
	}
	else
	{
		int32 i = 0;
		while(i < pid_count)
		{
			if(sendMsg(*(pids + i), msg) == SYSERR) //attempt to send message
			{
				kprintf("Sending message %s to process %d resulted in an error. Process ending.\n", msg, *(pids + i)); //queue is full, so end process
				return SYSERR;
			}
			else
			{
				i++; //queue is not full, so continue
			}

		}
		kprintf("Sent message %s to %d processes\n", msg, pid_count);
		return pid_count;
	}
}
int main(int argc, char **argv)
{
	//uint32 retval;
	//resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	umsg32 messages [MAXMESSAGES]; //array of messages to test sendMsg and receiveMsg methods
	messages[0] = "Hello";
	messages[1] = "My";
	messages[2] = "Name";
	messages[3] = "Is";
	messages[4] = "Beans";
	messages[5] = "And";
	messages[6] = "I";
	messages[7] = "Play";
	messages[8] = "Bridge";
	messages[9] = "Erry'day";
	sendMsg(getpid(), messages[0]);
	sendMsg(getpid(), messages[1]);
	sendMsg(getpid(), messages[2]);
	sendMsg(getpid(), messages[3]);
	sendMsg(getpid(), messages[4]);
	sendMsg(getpid(), messages[5]);
	sendMsg(getpid(), messages[6]);
	sendMsg(getpid(), messages[7]);
	sendMsg(getpid(), messages[8]);
	sendMsg(getpid(), messages[9]); //queue is full
	sleep(5);
	int32 i = 0;
	while(i < 10)
	{
		receiveMsg(); //empties the queue
		i++;
	}
	sleep(5);
	sendMsgs(getpid(), messages, 10); //send all the messages at once
	sleep(5);
	receiveMsgs(messages, 10); //receive all the messages at once
	sleep(5);
    pid32 processes [2];
    processes[0] = getpid();
    pid32 myPid = create(testMethod, 1024, 1, "testMethod", 0); //method that does nothing
    processes[1] = myPid;
    sendnMsg(2, processes, "Hello"); //sends message to batch of processes
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
void testMethod(void)
{

}

