#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

typedef unsigned long long uint64;

struct List{
    int i;
    int j;
    struct List *next;
    struct List *prev;
};

typedef struct taskPair{
    int i;
    int j;
} TaskPair;

struct List *createListNode(int i, int j)
{
    struct List *newNode
        = (struct List*)malloc(sizeof(struct List));
    newNode->i = i;
    newNode->j = j;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

uint64 myRand()
{
    uint64 highNum = rand();
    uint64 lowNum = rand();
    return (highNum << 16) + lowNum;
}

int main()
{
    void clearList(struct List *head);
    // int taskNum[2] = {6, 20};
    // int machNum[2] = {4, 10};
    // int diskNum[2] = {4, 10};
    // int taskSize[2] = {10, 600};
    // int dataSize[2] = {0, 20};
    // int machPower[2] = {1, 20};
    // int diskCapas[2] = {10000, 10500};
    // int diskSpeed[2] = {1, 20};
    // int depSize = 100;

    /*********************************/
    int taskNum[2] = {6, 10000};
    int machNum[2] = {1, 50};
    int diskNum[2] = {2, 30};
    int taskSize[2] = {10, 600};
    int dataSize[2] = {0, 20};
    int machPower[2] = {1, 20};
    int diskCapas[2] = {1, 1050000};
    int diskSpeed[2] = {1, 20};
    int depSize = 500000;

    if(access("input.txt", F_OK) == 0){
        remove("input.txt");
    }
    
    srand((unsigned)time(NULL));
    int task_num = rand() % (taskNum[1] - taskNum[0]) + taskNum[0];
    int mach_num = rand() % (machNum[1] - machNum[0]) + machNum[0];
    int disk_num = rand() % (diskNum[1] - diskNum[0]) + diskNum[0];

    int fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    printf("%d\n", task_num);
    fclose(stdout);
    fdopen(fd_stdout, "w");

    printf("Generating %d tasks...\n", task_num);
    fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    for(int i = 0; i < task_num; i++){
        int task_size = rand() % (taskSize[1] - taskSize[0]) + taskSize[0];
        int data_size = rand() % (dataSize[1] - dataSize[0]) + dataSize[0];
        int curMachNum = rand() % mach_num + 1;
        printf("%d %d %d %d ", i+1, task_size, data_size, curMachNum);
        struct List *head = createListNode(0, 0);
        struct List *node = head;
        for(int j = 0; j < mach_num; j++){
            struct List *newNode = createListNode(j, j);
            node->next = newNode;
            newNode->prev = node;
            node = newNode;
        }
        int k = 0;
        while(k < curMachNum){
            int pos = rand() % (mach_num-k);
            int count = 0;
            node = head->next;
            while(count < pos){
                node = node->next;
                count += 1;
            }
            printf("%d ", node->i+1);
            struct List *prevNode = node->prev;
            struct List *nextNode = node->next;
            prevNode->next = nextNode;
            if(nextNode != NULL){
                nextNode->prev = prevNode;
            }
            free(node);
            k += 1;
        }
        clearList(head);
        putchar('\n');
    }
    fclose(stdout);
    fdopen(fd_stdout, "w");
    printf("Done.\n");

    printf("Generating %d machines...\n", mach_num);
    fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    printf("%d\n", mach_num);
    for(int i = 0; i < mach_num; i++){
        int mach_power = rand() % (machPower[1] - machPower[0]) + machPower[0];
        printf("%d %d \n", i+1, mach_power);
    }
    fclose(stdout);
    fdopen(fd_stdout, "w");
    fputs("Done.\n", stdout);

    printf("Generating %d disks...\n", disk_num);
    fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    printf("%d\n", disk_num);
    for(int i = 0; i < disk_num; i++){
        int disk_speed = rand() % (diskSpeed[1] - diskSpeed[0]) + diskSpeed[0];
        uint64 disk_capas = myRand() % (diskCapas[1] - diskCapas[0]) + diskCapas[0];
        printf("%d %d %llu \n", i+1, disk_speed, disk_capas);
    }
    fclose(stdout);
    fdopen(fd_stdout, "w");
    fputs("Done.\n", stdout);

    int allStateNum = task_num * (task_num-1) / 2;
    int dep_size = (allStateNum < depSize) ? allStateNum : depSize;
    int totalDepSize = myRand() % dep_size+1;
    int dataDepSize = myRand() % totalDepSize+1;
    int taskDepSize = totalDepSize - dataDepSize;

    printf("Generating %d data dependencies...\n", dataDepSize);
    fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    printf("%d\n", dataDepSize);
    TaskPair *stateArray = (TaskPair*)malloc(sizeof(TaskPair) * allStateNum);
    int k = 0;
    for(int i = 0; i < task_num-1; i++){
        for(int j = i+1; j < task_num; j++){
            stateArray[k].i = i;
            stateArray[k].j = j;
            k += 1;
        }
    }
    for(int i = 0; i < totalDepSize; i++){
        uint64 randomIndex = myRand() % allStateNum;
        TaskPair temp = stateArray[randomIndex];
        stateArray[randomIndex] = stateArray[i];
        stateArray[i] = temp;
    }
    TaskPair *totalDepArray = (TaskPair*)malloc(sizeof(TaskPair) * totalDepSize);
    for(int i = 0; i < totalDepSize; i++){
        TaskPair node = stateArray[i];
        totalDepArray[i].i = node.i;
        totalDepArray[i].j = node.j;
    }
    free(stateArray);
    for(int i = 0; i < dataDepSize; i++){
        uint64 randomIndex = myRand() % totalDepSize;
        TaskPair temp = totalDepArray[randomIndex];
        totalDepArray[randomIndex] = totalDepArray[i];
        totalDepArray[i] = temp;
    }
    for(int i = 0; i < dataDepSize; i++){
        TaskPair node = totalDepArray[i];
        printf("%d %d\n", node.i+1, node.j+1);
    }
    fclose(stdout);
    fdopen(fd_stdout, "w");
    fputs("Done.\n", stdout);

    printf("Generating %d task dependencies...\n", taskDepSize);
    fd_stdout = dup(fileno(stdout));
    freopen("input.txt", "a", stdout);
    printf("%d\n", taskDepSize);
    for(int i = dataDepSize; i < totalDepSize; i++){
        TaskPair node = totalDepArray[i];
        printf("%d %d\n", node.i+1, node.j+1);
    }
    fclose(stdout);
    free(totalDepArray);
    fdopen(fd_stdout, "w");
    fputs("Done.\n", stdout);
    return 0;
}

void clearList(struct List *head)
{
    struct List *node;
    while(head != NULL){
        node = head;
        head = head->next;
        free(node);
    }
}