#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define ERR    (-1)             /* indicates an error condition */
#define READ   0                /* read end of a pipe */
#define WRITE  1                /* write end of a pipe */
#define STDIN  0                /* file descriptor of standard in */
#define STDOUT 1                /* file descriptor of standard out */
/* This code reads in a message from the user, creates a parent process that sends it to a child process, which reverses its input, sends that message
to another child process, which capitalizes it, and then sends it back to the parent process to print out. To accomplish this, three pipes were created, as well
as three processes. Each pipe has read and write ends that need to be carefully handled in order to prevent errors arising. */
int main()
{
	pid_t pid_1,               /* will be process id of first child, which inverts the string */
	pid_2;               /* will be process id of second child, which converts the string to uppercase */
	int fd[2]; //descriptor array for parent process
	int fd2[2]; //descriptor array for first child process
	int fd3[2]; //descriptor array for second child process
	char ch [100]; //original character array
	char ch2 [100]; //character array after reversal
	int index = 0; //size
	ssize_t z = 0; //error condition
	char character;
	while((character = getchar()) != '\n') //get input and put it into array
	{
		ch[index] = character;
		index++;
	}
	ch[index+1] = '\0';
	index++;
	if(pipe (fd) == ERR)
	{
		perror("Parent pipe cannot be created\n");
		exit (ERR);
	}
	if (pipe (fd2) == ERR)              /* create a pipe  */
	{                                 /* must do before a fork */
		perror ("First child pipe cannot be created.\n");
		exit (ERR);
	}
	if (pipe (fd3) == ERR)              /* create a pipe  */
	{                                 /* must do before a fork */
		perror ("Second child pipe cannot be created.\n");
		exit (ERR);
	}
	if ((pid_1 = fork()) == ERR)        /* create 1st child   */
	{
		perror ("First child process cannot be created.\n");
		exit (ERR);
	}
	if (pid_1 != 0)                      /* in parent  */
	{
		close(fd2[0]); //close read end of first child
		printf("Parent process sends message %s\n", ch);
		z = write(fd2[1], ch, sizeof(ch)); //write to write end of first child
		if(z != -1)
		{
			close(fd2[1]); //close write end of first child
		}
		else //error
		{
			printf("Error %d has emerged with the parent process sending the message\n",errno);
		}
		if ((pid_2 = fork ()) == ERR)     /* create 2nd child  */
		{
			perror ("Second child process cannot be created.\n");
			exit (ERR);
		}
		if (pid_2 != 0)                   /* still in parent  */
		{
			wait ((int *) 0);           /* wait for children to die */
			wait ((int *) 0);
			z = read(fd[0], ch2, sizeof(ch2)); //read read end of parent process
			if(z != -1)
			{
				printf("Parent process receives message %s\n", ch2);
				int i = 0;
				while (i < index)
				{
					printf("%c", ch2[i]); //print message
					i++;
				}
				printf("\n");
				close(fd3[1]); //close write end of second child
				close(fd[0]); //close read end of parent process
			}
			else //error
			{
				printf("Error %d has emerged with the parent process reading the message\n",errno);
			}
		}
		else                                /* in 2nd child   */
		{
			close(fd3[1]); //close write end of second child
			close(fd2[0]); //close read end of first child
			z = read(fd3[0], ch2, sizeof(ch2)); //read read end of second child
			if(z != -1)
			{
				printf("Second child receives %s\n", ch2);
				int i = 0;
				while (i < index)
				{
					ch2[i] = toupper(ch2[i]); //convert to uppercase
					i ++;
				}
			}
			else //error
			{
				printf("Error %d has emerged with the second child process reading the message\n",errno);
			}
			z = write(fd[1],ch2, sizeof(ch2)); //write to write end of parent process
			if(z != -1)
			{
				printf("Second child sends message %s\n", ch2);
				close(fd3[0]); //close read end of second child
				close(fd[1]); //close write end of parent process
			}
			else //error
			{
				printf("Error %d has emerged with the second child process sending the message\n",errno);
			}
		}
	}
	else                                      /* in 1st child   */
	{
		close(fd2[1]); //close write end of first child
		close(fd[0]); //close read end of parent process
		z = read(fd2[0], ch, sizeof(ch)); //read read end of first child
		if(z != -1)
		{
			printf("First child receives message %s\n", ch);
			int i = 0;
			while (i < index - 1)
			{
				ch2[i] = ch[index - 2 - i]; //reverse
				i++;
			}
			ch2[index] = '\0';
		}
		else //error
		{
			printf("Error %d has emerged with the first child process reading the message\n",errno);
		}
		z = write(fd3[1], ch2, sizeof(ch2)); //write to write end of second child
		if (z != -1)
		{
			printf("First child sends message %s\n", ch2);
			close(fd3[1]); //close write end of second child
			close(fd2[0]); //close read end of first child
		}
		else //error
		{
			printf("Error %d has emerged with the first child process sending the message\n",errno);
		}
	}
	exit(0);
}
