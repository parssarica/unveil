#include <stdio.h>
#include <unistd.h>
#include "unveilptrace.h"

int main(void)
{
    FILE* fd;
    char content[256];
/*    printf("gonna sleep...\n");
    sleep(30);
    printf("sleep completed.\n");
  */  //pledge("stdio wpath", NULL);
    unveil("unveil.txt", "r");
    unveil("../example.c", "r");
    unveil(NULL, NULL);

    fd = fopen("../example.c", "rt");
    if(fd == NULL)
    {
        printf("fd == NULL\n");
    }
    fgets(content, 256, fd);
    printf("%s\n", content);
    fclose(fd);

    fd = fopen("../example.c", "rb");
    if(fd == NULL)
    {
        printf("fd == NULL\n");
    }
    fgets(content, 256, fd);
    printf("%s\n", content);
    fclose(fd);

    fd = fopen("/tmp/exampletmp.c", "w+t");
    if(fd == NULL)
    {
        printf("fd == NULL\n");
    }
    fgets(content, 256, fd);
    printf("%s\n", content);
    fclose(fd);



    fd = fopen("/tmp/exampletmp.c", "w+b");
    if(fd == NULL)
    {
        printf("fd == NULL\n");
    }
    fgets(content, 256, fd);
    printf("%s\n", content);
    fclose(fd);

    fd = fopen("/tmp/exampletmp.c", "wb");
    if(fd == NULL)
    {
        printf("fd == NULL\n");
    }
    fgets(content, 256, fd);
    printf("%s\n", content);
    fclose(fd);


    return 0;
}
