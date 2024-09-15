#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "queuesage.h"

msg msg_obj;
int unveil(char* file, char* modes)
{
    char *json_str;
    char abp[PATH_MAX];

    if(setup(&msg_obj, "/queuesage1") == (mqd_t)-1)
    {
        perror("mq_open");
        exit(1);
    }

    if(file == NULL || modes == NULL)
    {
        if(send_msg(&msg_obj, "unveil_ended", 0) == -1)
        {
            exit(1);
        }
        return 0;
    }

    if(realpath(file, abp)==NULL) abp[0] = 0;

    asprintf(&json_str, "%s|%s", abp, modes);
//    printf("from unvelptrace.c: json_str:<%s>\n", json_str);
 
    if(send_msg(&msg_obj, json_str, 0) == -1)
    {
        free(json_str);
        exit(1);
    }

    free(json_str);
    return 0;
}

