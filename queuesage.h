#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

typedef struct _msg
{
    mqd_t mq;
    struct mq_attr attr;
    char buffer[256];
}msg;

int setup(msg* message, char* qn);
int send_msg(msg* message, char* msg, int priority);
int receive(msg* message, unsigned int* priority, char** msg);
int terminate_queue(msg* message);
