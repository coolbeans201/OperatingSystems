#include <xinu.h>
#include <stdio.h>

#define LEFT (n+4)%5 //the fork on the left, with the modulus to keep it circular
#define RIGHT (n+1)%5 //the fork on the right, with the modulus to keep it circular
#define THINKING 0
#define HUNGRY 1
#define EATING 2

void philosopher(int32 n);
void pickuptest(int32 n);
void pickup(int32 n);
void putdown(int32 n);
void think(int32 n);
void eat(int32 n);
void printer();

sid32 waiter; //semaphore in charge of serving food
sid32 print; //semaphore in charge of printing data
sid32 mutex; //mutex semaphore
int32 state[5]; //philosopher state array
sid32 forks[5]; //fork array
int32 times_eaten[5]; //times eaten array

/* The philosophers can only pick up one fork at a time. Additionally,
 * both forks need to be picked up in order for that philosopher
 * to eat at all. By doing this, along with sleeping between
 * some operations, there can be no interference between the philosophers.
 * Combine this with a mutex semaphore and you have mutually-exclusive,
 * deadlock-free code.
 */
int main(int argc, char **argv) {
    waiter = semcreate(1); //creates waiter semaphore
    print = semcreate(1); //creates print semaphore
    mutex = semcreate(1); //creates mutex semaphore
    int32 i = 0;
    for (i = 0; i < 5; i++) {
        times_eaten[i] = 0; //initializes time eaten to 0 for every philosopher
    }

    i = 0;
    for (i = 0; i < 5; i++) {
        forks[i] = semcreate(1); //creates a semaphore for each fork
    }

    for (i = 0; i < 5; i++) {
        resume(create(philosopher, 4096, 50, "philosopher", 1, i)); //creates process for each philosopher
    }

    resume(create(printer, 4096, 50, "print", 0)); //print process
}

void philosopher(int32 n) {
    kprintf("Philosopher %d created\n", n);
    while (TRUE) {
        think(n);
		wait(mutex);
        pickup(n);
        eat(n);
        putdown(n);
		signal(mutex);
    }
}

void pickup(int32 n) {
    wait(waiter); //decrement waiter count
    if (state[n] != HUNGRY) {
        state[n] = HUNGRY; //set state to hungry
        kprintf("Philosopher %d is hungry\n", n);
    }
    pickuptest(n); //test to see if forks can be picked up
    signal(mutex); //increment mutex count
    signal(waiter); //increment waiter count
}

void putdown(int32 n) {
    wait(waiter); //decrement waiter count
    state[n] = THINKING; //set state to thinking since done with food
    signal(forks[n]); //increment semaphore count
    signal(forks[RIGHT]); //increment semaphore count
    kprintf("Philosopher %d put down forks %d and %d\n", n, n, RIGHT);
    pickuptest(LEFT); //test to see if left fork can be picked up
    pickuptest(RIGHT); //test to see if right fork can be picked up
    signal(mutex); //increment mutex count
    signal(waiter); //increment waiter count
}

void pickuptest(int32 n) {
    if (state[n] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING) {
        wait(forks[n]); //decrement semaphore count
        wait(forks[RIGHT]); //decrement semaphore count
        kprintf("Philosopher %d took forks %d and %d\n", n, n, RIGHT);
        state[n] = EATING; //set state to eating since both forks are available
    }
}

void think(int32 n) {
    if (state[n] == THINKING) {
        kprintf("Philosopher %d is thinking\n", n);
        sleep(3);
    }
}

void eat(int32 n) {
    if (state[n] == EATING) {
        kprintf("Philosopher %d is eating\n", n);
        times_eaten[n]++; //increment times eaten
    }
}

void printer() {
    while (TRUE) {
        sleep(5);
        wait(waiter); //decrement waiter count
        wait(mutex); //decrement mutex count
        kprintf("\nSummary: \n   Philosopher  |   Times Eaten\n");
        int32 i = 0;
        for (i = 0; i < 5; i++) {
            kprintf("\t%d\t|\t%d\n", i, times_eaten[i]);
        }
        kprintf("\n");
        signal(mutex); //increment mutex count
        signal(waiter); //increment waiter count
    }
}



