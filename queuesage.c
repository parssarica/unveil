#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

#define MAX_MSG_SIZE 16384

typedef struct _msg
{
    mqd_t mq;
    struct mq_attr attr;
    char buffer[256];
}msg;

int setup(msg* message, char* qn)
{
    char buffer[MAX_MSG_SIZE];
    ssize_t bytes_read;
    message->attr.mq_flags = 0;
    message->attr.mq_maxmsg = 10;
    message->attr.mq_msgsize = 256;
    message->attr.mq_curmsgs = 0;

    message->mq = mq_open(qn, O_RDWR | O_CREAT | O_NONBLOCK, S_IRUSR | S_IWUSR, &(message->attr));
    if(message->mq == (mqd_t)-1)
    {
        perror("mq_open");
        return (mqd_t)-1;
    }

    while (1)
    {
        bytes_read = mq_receive(message->mq, buffer, MAX_MSG_SIZE, NULL);
        if (bytes_read >= 0)
        {
            printf("Cleared message: %s\n", buffer);
        }
        else
        {
            if (errno == ENOMSG)
            {
                printf("no more messages\n");
                break;
            }
            else
            {
                break;
            }
        }
    }

    return message->mq;
}

int send_msg(msg* message, char* msg, int priority)
{
    strncpy(message->buffer, msg, strlen(msg) + 1);
    if(mq_send(message->mq, message->buffer, sizeof(message->buffer), priority) == -1)
    {
        perror("mq_send");
        return -1;
    }
    return 0;
}

int receive(msg* message, unsigned int* priority, char** msg)
{
    int bytes_received = mq_receive(message->mq, message->buffer, MAX_MSG_SIZE, priority);
    if(errno == ENOMSG || bytes_received <= 0)
    {
        return 0;
    }

    if(bytes_received == -1 && errno != EAGAIN)
    {
        perror("mq_receive");
        return -1;
    }
    //*msg = realloc(msg, bytes_received);
    *msg = malloc(bytes_received);
    strncpy(*msg, message->buffer, bytes_received);
    return bytes_received;
}

int terminate_queue(msg* message)
{
    if(mq_close(message->mq) == -1)
    {
        perror("mq_close");
        return -1;
    }
    return 0;
}
