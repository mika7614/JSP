#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#define ROUND_DIV(a, b) (a / b + (a % b == 0 ? 0 : 1))
#define MAXNUM 1000

typedef struct rawData{
    char **ts;
    char **ms;
    char **disks;
    char **task_dp;
    char **envir_dp;
    int task_num;
    int ms_num;
    int disk_num;
    int task_dp_num;
    int task_envir_num;
} RawData;

typedef struct taskMachineMap{
    int nums;
    int *machines;
} TaskMachineMap;

typedef struct variables{
    int *task_costs;       // 1d array: task_id -> task_size
    int *data_costs;       // 1d array: task_id -> data_size
    TaskMachineMap *task_machine_dp; // 1d array: {nums, available_machine}
    int *ms_power;         // 1d array: machine_id -> power
    int *disk_speeds;      // 1d array: disk_id -> speed
    int *disk_quotas;      // 1d array: disk_id -> quota
    int **task_data_dps;   // 2d array: List of [parent, child] (Task constraint w.r.t. data)
    int **task_envir_dps;  // 2d array: List of [parent, child] (Task constraint w.r.t. environment)
} Variables;

typedef struct answer{
    int startTime;
    int taskEndTime;
    int dataEndTime;
    int machineID;
    int diskID;
    int parentMaxTime;
} Answer;

// Structure to represent a stack
struct Stack{
    int data;
    struct Stack *next;
};

// Pointer to an array containing adjacency lists
struct Graph{
    int V; // No. of vertices
    struct List *adj;
};

// Structure to represent a list (adjacency list)
struct List{
    int data;
    struct List *next;
};

typedef struct taskSchedule{
    int taskID;
    int startTime;
    int endTime;
} TaskSchedule;

int main()
{
    void read_raw_data(RawData *contents);
    void output_variables(RawData contents, Variables src);
    void preprocess(RawData *contents, Variables *src);
    char **readOutputRawData(int task_num);
    Answer *convertAns(char **chsAns, int task_num);
    void clearChsAns(char **chsAns, int task_num);
    bool checkAnswer(Answer *ans, RawData contents, Variables src);

    freopen("input.txt", "r", stdin);

    RawData contents;
    read_raw_data(&contents);
    Variables src;
    preprocess(&contents, &src);

    fclose(stdin);

    // output_variables(contents, src);

    freopen("output.txt", "r", stdin);
    char **chsAns = readOutputRawData(contents.task_num);
    Answer *ans = convertAns(chsAns, contents.task_num);
    clearChsAns(chsAns, contents.task_num);
    if(checkAnswer(ans, contents, src)){
        printf("Correct!");
    }
    fclose(stdin);
    return 0;
}

bool checkAnswer(Answer *ans, RawData contents, Variables src)
{
    void calcAllEndTime(RawData contents, Variables src, Answer *ans);
    bool checkDiskQuotas(Answer *ans, RawData contents, Variables src);
    bool checkTaskDataDps(RawData contents, Variables src, Answer *ans);
    bool checkEnvirDps(RawData contents, Variables src, Answer *ans);
    bool checkOverlap(RawData contents, Answer *ans);
    bool checkAffinity(RawData contents, Variables src, Answer *ans);

    calcAllEndTime(contents, src, ans);
    if(!checkDiskQuotas(ans, contents, src)){
        printf("Disk quotas are wrong\n");
        return false;
    }
    if(!checkTaskDataDps(contents, src, ans)){
        printf("Something wrong in data dependencies of tasks\n");
        return false;
    }
    if(!checkEnvirDps(contents, src, ans)){
        printf("Something wrong in environment dependencies of tasks\n");
        return false;
    }
    if(!checkOverlap(contents, ans)){
        printf("Something wrong in preemption\n");
        return false;
    }
    if(!checkAffinity(contents, src, ans)){
        printf("Something wrong in affinity\n");
        return false;
    }
    return true;
}

bool checkAffinity(RawData contents, Variables src, Answer *ans)
{
    for(int i = 0; i < contents.task_num; i++){
        int machineID = ans[i].machineID;
        int *avail_machines = src.task_machine_dp[i].machines;
        int avail_nums = src.task_machine_dp[i].nums;
        bool flag = false;
        for(int j = 0; j < avail_nums; j++){
            if(avail_machines[j] == machineID){
                flag = true;
                break;
            }
        }
        if(!flag){
            printf("taskID:%d, machineID:%d\n", i+1, machineID+1);
            return false;
        }
    }
    return true;
}

void calcAllEndTime(RawData contents, Variables src, Answer *ans)
{
    void clearChildrenToParents(struct List **childrenToParents, int task_num);
    int calcPrevDataTime(Variables src, Answer *ans, struct List *parent);
    struct List *createListNode(int data);
    struct List **childrenToParents = (struct List**)malloc(contents.task_num * sizeof(struct List*));

    for(int i = 0; i < contents.task_num; i++){
        childrenToParents[i] = NULL;
    }
    for(int i = 0; i < contents.task_dp_num; i++){
        int parentID = src.task_data_dps[i][0];
        int childID = src.task_data_dps[i][1];
        struct List *newNode = createListNode(parentID);
        newNode->next = childrenToParents[childID];
        childrenToParents[childID] = newNode;
    }
    for(int taskID = 0; taskID < contents.task_num; taskID++){
        int startTime = ans[taskID].startTime;
        struct List *parent = childrenToParents[taskID];
        // calculate time b
        int prevDataTime = calcPrevDataTime(src, ans, parent);
        // calculate time c
        int machineID = ans[taskID].machineID;
        int power = src.ms_power[machineID];
        int taskCost = src.task_costs[taskID];
        ans[taskID].taskEndTime = startTime + prevDataTime + ROUND_DIV(taskCost, power);
        // calculate time d
        int dataCost = src.data_costs[taskID];
        int diskSpeed = src.disk_speeds[ans[taskID].diskID];
        ans[taskID].dataEndTime = ans[taskID].taskEndTime + ROUND_DIV(dataCost, diskSpeed);
        // printf("taskID:%d, startTime: %d, prevDataTime: %d, taskEndTime:%d, dataEndTime: %d\n",
        //         taskID+1, startTime, prevDataTime, ans[taskID].taskEndTime, ans[taskID].dataEndTime);
    }
    clearChildrenToParents(childrenToParents, contents.task_num);
}

int calcPrevDataTime(Variables src, Answer *ans, struct List *parent)
{
    int prevDataTime = 0;
    while(parent != NULL){
        int parentID = parent->data;
        int dataCost = src.data_costs[parentID];
        int diskID = ans[parentID].diskID;
        int diskSpeed = src.disk_speeds[diskID];
        prevDataTime += ROUND_DIV(dataCost, diskSpeed);
        parent = parent->next;
    }
    return prevDataTime;
}

bool checkOverlap(RawData contents, Answer *ans)
{
    bool calcOverlap(int curMachineID, Answer *ans, RawData contents, int onlineTaskNum);
    int *counter = (int*)malloc(contents.ms_num * sizeof(int));
    memset(counter, 0, contents.ms_num * sizeof(int));

    for(int i = 0; i < contents.task_num; i++){
        int machineID = ans[i].machineID;
        counter[machineID] += 1;
    }
    for(int i = 0 ; i < contents.ms_num; i++){
        if(counter[i] < 1){
            continue;
        }
        if(!calcOverlap(i, ans, contents, counter[i])){
            return false;
        }
    }
    return true;
}

bool calcOverlap(int curMachineID, Answer *ans, RawData contents, int onlineTaskNum)
{
    TaskSchedule *plan = (TaskSchedule*)malloc(onlineTaskNum * sizeof(TaskSchedule));
    int j = 0;
    for(int taskID = 0; taskID < contents.task_num; taskID++){
        int machineID = ans[taskID].machineID;
        if(machineID != curMachineID){
            continue;
        }
        plan[j].startTime = ans[taskID].startTime;
        plan[j].endTime = ans[taskID].dataEndTime;
        plan[j].taskID = taskID;
        j += 1;
    }
    for(int i = 0; i < onlineTaskNum-1; i++){
        for(int j = i+1; j < onlineTaskNum; j++){
            int a, b;
            a = plan[i].startTime;
            b = plan[j].startTime;
            int startUpper = (a > b) ? a : b;
            a = plan[i].endTime;
            b = plan[j].endTime;
            int endLower = (a < b) ? a : b;
            if(startUpper < endLower){
                printf("MachineID: %d, task1: %d, task2: %d\n", curMachineID+1,
                        plan[i].taskID+1, plan[j].taskID+1);
                return false;
            }
        }
    }
    return true;
}

bool checkEnvirDps(RawData contents, Variables src, Answer *ans)
{
    bool checkParentStartTimeEnvirDps(struct List *parent, int childStartTime, Answer *ans);
    void clearChildrenToParents(struct List **childrenToParents, int task_num);
    struct List *createListNode(int data);
    struct List **childrenToParents = (struct List**)malloc(contents.task_num * sizeof(struct List*));
    for(int i = 0; i < contents.task_num; i++){
        childrenToParents[i] = NULL;
    }

    for(int i = 0; i < contents.task_envir_num; i++){
        int parentID = src.task_envir_dps[i][0];
        int childID = src.task_envir_dps[i][1];
        struct List *newNode = createListNode(parentID);
        newNode->next = childrenToParents[childID];
        childrenToParents[childID] = newNode;
    }

    for(int taskID = 0; taskID < contents.task_num; taskID++){
        int startTime = ans[taskID].startTime;
        struct List *parent = childrenToParents[taskID];
        if(!checkParentStartTimeEnvirDps(parent, startTime, ans)){
            clearChildrenToParents(childrenToParents, contents.task_num);
            return false;
        }
    }
    clearChildrenToParents(childrenToParents, contents.task_num);
    return true;
}

bool checkParentStartTimeEnvirDps(struct List *parent, int childStartTime, Answer *ans)
{
    while(parent != NULL){
        int parentID = parent->data;
        int parentEndTime = ans[parentID].taskEndTime;
        if(parentEndTime > childStartTime){
            return false;
        }
        parent = parent->next;
    }
    return true;
}

bool checkTaskDataDps(RawData contents, Variables src, Answer *ans)
{
    bool checkParentStartTimeDataDps(struct List *parent, int childStartTime,
                                     Answer *ans);
    void clearChildrenToParents(struct List **childrenToParents, int task_num);
    struct List *createListNode(int data);
    struct List **childrenToParents = (struct List**)malloc(contents.task_num * sizeof(struct List*));
    for(int i = 0; i < contents.task_num; i++){
        childrenToParents[i] = NULL;
    }
    for(int i = 0; i < contents.task_dp_num; i++){
        int parentID = src.task_data_dps[i][0];
        int childID = src.task_data_dps[i][1];
        struct List *newNode = createListNode(parentID);
        newNode->next = childrenToParents[childID];
        childrenToParents[childID] = newNode;
    }

    for(int taskID = 0; taskID < contents.task_num; taskID++){
        int startTime = ans[taskID].startTime;
        struct List *parent = childrenToParents[taskID];
        if(!checkParentStartTimeDataDps(parent, startTime, ans)){
            printf("childStartTime is %d\n", startTime);
            printf("childID is %d\n", taskID+1);
            clearChildrenToParents(childrenToParents, contents.task_num);
            return false;
        }
    }
    clearChildrenToParents(childrenToParents, contents.task_num);
    return true;
}

bool checkParentStartTimeDataDps(struct List *parent, int childStartTime, Answer *ans)
{
    while(parent != NULL){
        int parentID = parent->data;
        int parentEndTime = ans[parentID].dataEndTime;
        if(parentEndTime > childStartTime){
            printf("parentEndTime is %d\n", parentEndTime);
            printf("parentID is %d\n", parentID+1);
            return false;
        }
        parent = parent->next;
    }
    return true;
}

void clearChildrenToParents(struct List **childrenToParents, int task_num)
{
    for(int i = 0; i < task_num; i++){
        struct List *node = childrenToParents[i];
        if(node != NULL){
            struct List *temp = node;
            node = node->next;
            free(temp);
        }
    }
}

bool checkDiskQuotas(Answer *ans, RawData contents, Variables src)
{
    int task_num = contents.task_num;
    int disk_num = contents.disk_num;
    int *data_costs = src.data_costs;
    int *disk_quotas = src.disk_quotas;
    int *tempDiskQuotas = (int*)malloc(disk_num * sizeof(int));
    for(int i = 0; i < disk_num; i++){
        tempDiskQuotas[i] = disk_quotas[i];
    }
    for(int taskID = 0; taskID < task_num; taskID++){
        int diskID = ans[taskID].diskID;
        tempDiskQuotas[diskID] -= data_costs[taskID];
        if(tempDiskQuotas[diskID] < 0){
            printf("Disk %d out of space\n", diskID+1);
            free(tempDiskQuotas);
            return false;
        }
    }
    free(tempDiskQuotas);
    return true;
}

Answer *convertAns(char **chsAns, int task_num)
{
    // Reorder all tasks
    int *ch_to_digit(char *chs);

    Answer *ans = (Answer*)malloc(task_num * sizeof(Answer));
    for(int i = 0; i < task_num; i++){
        int *digits = ch_to_digit(chsAns[i]);
        int taskID = digits[0]-1;
        ans[taskID].startTime = digits[1];
        ans[taskID].machineID = digits[2]-1;
        ans[taskID].diskID = digits[3]-1;
        free(digits);
    }
    return ans;
}

void clearChsAns(char **chsAns, int task_num)
{
    for(int i = 0; i < task_num; i++){
        free(chsAns[i]);
    }
    free(chsAns);
}

void read_raw_data(RawData *contents)
{
    char **load_data(int a);

    // // Load Tasks
    scanf("%d", &contents->task_num);
    while(getchar() != '\n');
    contents->ts = load_data(contents->task_num);

    // Load Machines
    scanf("%d", &contents->ms_num);
    while(getchar() != '\n');
    contents->ms = load_data(contents->ms_num);

    // Load Disks
    scanf("%d", &contents->disk_num);
    while(getchar() != '\n');
    contents->disks = load_data(contents->disk_num);

    // Load Task-Dependencies
    scanf("%d", &contents->task_dp_num);
    while(getchar() != '\n');
    contents->task_dp = load_data(contents->task_dp_num);

    // Load Environment-Dependencies
    scanf("%d", &contents->task_envir_num);
    while(getchar() != '\n');
    contents->envir_dp = load_data(contents->task_envir_num);
}

char **readOutputRawData(int task_num)
{
    char **load_data(int a);
    char **chsAns;

    chsAns = load_data(task_num);
    return chsAns;
}

void preprocess(RawData *contents, Variables *src)
{
    void build_TD_cost_and_TM_dp(int task_num, char **ts, int *task_costs,
                                 int *data_costs, TaskMachineMap *task_machine_dp);
    void build_ms_power(int ms_num, char **ms, int *ms_power);
    void build_disk_stats(int disk_num, char **disks, int *disk_speeds,
                          int *disk_quotas);
    void build_task_data_dps(int task_dp_num, char **task_dp, int **task_data_dps);
    void build_task_envir_dps(int task_envir_num, char **envir_dp, int **task_envir_dps);
    void clear_raw_data(RawData *contents);

    src->task_costs = (int*)malloc(contents->task_num * sizeof(int));
    src->data_costs = (int*)malloc(contents->task_num * sizeof(int));
    src->task_machine_dp = (TaskMachineMap*)malloc(contents->task_num * sizeof(TaskMachineMap));
    build_TD_cost_and_TM_dp(contents->task_num, contents->ts,
                            src->task_costs, src->data_costs, src->task_machine_dp);

    src->ms_power = (int*)malloc(contents->ms_num * sizeof(int));
    build_ms_power(contents->ms_num, contents->ms, src->ms_power);

    src->disk_speeds = (int*)malloc(contents->disk_num * sizeof(int));
    src->disk_quotas = (int*)malloc(contents->disk_num * sizeof(int));
    build_disk_stats(contents->disk_num, contents->disks, src->disk_speeds, src->disk_quotas);

    src->task_data_dps = (int**)malloc(contents->task_dp_num * sizeof(int*));
    build_task_data_dps(contents->task_dp_num, contents->task_dp,
                        src->task_data_dps);

    src->task_envir_dps = (int**)malloc(contents->task_envir_num * sizeof(int*));
    build_task_envir_dps(contents->task_envir_num, contents->envir_dp,
                         src->task_envir_dps);

    clear_raw_data(contents);
}

void clear_raw_data(RawData *contents)
{
    for(int i = 0; i < contents->task_num; i++){
        free((contents->ts)[i]);
    }
    free(contents->ts);

    for(int i = 0; i < contents->ms_num; i++){
        free((contents->ms)[i]);
    }
    free(contents->ms);

    for(int i = 0; i < contents->disk_num; i++){
        free((contents->disks)[i]);
    }
    free(contents->disks);

    for(int i = 0; i < contents->task_dp_num; i++){
        free((contents->task_dp)[i]);
    }
    free(contents->task_dp);

    for(int i = 0; i < contents->task_envir_num; i++){
        free((contents->envir_dp)[i]);
    }
    free(contents->envir_dp);
}

void output_variables(RawData contents, Variables src)
{
    printf("%d\n", contents.task_num);
    for(int i = 0; i < contents.task_num; i++){
        printf("%d ", src.task_costs[i]);
    }
    putchar('\n');

    for(int i = 0; i < contents.task_num; i++){
        printf("%d ", src.data_costs[i]);
    }
    putchar('\n');

    printf("Task to machines:\n");
    for(int i = 0; i < contents.task_num; i++){
        printf("taskID: %d\n", i+1);
        for(int j = 0; j < src.task_machine_dp[i].nums; j++){
            printf("machineID:%d ", src.task_machine_dp[i].machines[j]+1);
        }
        putchar('\n');
    }

    for(int i = 0; i < contents.ms_num; i++){
        printf("%d ", src.ms_power[i]);
    }
    putchar('\n');

    for(int i = 0; i < contents.disk_num; i++){
        printf("%d ", src.disk_speeds[i]);
    }
    putchar('\n');

    for(int i = 0; i < contents.disk_num; i++){
        printf("%d ", src.disk_quotas[i]);
    }
    putchar('\n');

    printf("Task dependencies w.r.t. data:\n");
    for(int i = 0; i < contents.task_dp_num; i++){
        int parent = src.task_data_dps[i][0];
        int child = src.task_data_dps[i][1];
        printf("(%d, %d)\n", parent, child);
    }

    printf("Task dependencies w.r.t. environment:\n");
    for(int i = 0; i < contents.task_envir_num; i++){
        int parent = src.task_envir_dps[i][0];
        int child = src.task_envir_dps[i][1];
        printf("(%d, %d)\n", parent, child);
    }
}

char **load_data(int num)
{
    int i;
    char **chs, *ch;
    chs = (char**)malloc(num * sizeof(char*));

    for(i = 0; i < num; i++){
        ch = (char*)malloc(MAXNUM * sizeof(char));
        fgets(ch, MAXNUM, stdin);
        chs[i] = ch;
    }
    return chs;
}

void build_TD_cost_and_TM_dp(int task_num, char **ts, int *task_costs,
                             int *data_costs, TaskMachineMap *task_machine_dp)
{
    int *ch_to_digit(char *chs);
    for(int i = 0; i < task_num; i++){
        char *t = ts[i];
        int *digits = ch_to_digit(t);
        int task_id = digits[0]-1;
        int task_size = digits[1], data_size = digits[2];
        task_costs[task_id] = task_size;
        data_costs[task_id] = data_size;

        int curMsNum = digits[3];
        int *p = digits+4;
        int *temp = (int*)malloc(curMsNum * sizeof(int));
        memset(temp, -1, curMsNum * sizeof(int));
        int j = 0;
        while(*p != '\0'){
            int machine_id = *p-1;
            temp[j++] = machine_id;
            p++;
        }
        task_machine_dp[task_id].nums = curMsNum;
        task_machine_dp[task_id].machines = temp;
        free(digits);
    }
}

void build_ms_power(int ms_num, char **ms, int *ms_power)
{
    int *ch_to_digit(char *chs);
    int i;
    for(i = 0; i < ms_num; i++){
        char *t = ms[i];
        int *ans = ch_to_digit(t);
        int machine_id = ans[0]-1;
        ms_power[machine_id] = ans[1];
        free(ans);
    }
}

void build_disk_stats(int disk_num, char **disks, int *disk_speeds, int *disk_quotas)
{
    int *ch_to_digit(char *chs);
    int i;
    for(i = 0; i < disk_num; i++){
        char *t = disks[i];
        int *ans = ch_to_digit(t);
        int disk_id = ans[0]-1;
        disk_speeds[disk_id] = ans[1];
        disk_quotas[disk_id] = ans[2];
        free(ans);
    }
}

void build_task_data_dps(int task_dp_num, char **task_dp, int **task_data_dps)
{
    int *ch_to_digit(char *chs);
    int i;
    for(i = 0; i < task_dp_num; i++){
        int *temp = (int*)malloc(2 * sizeof(int));
        memset(temp, 0, 2 * sizeof(int));
        task_data_dps[i] = temp;
    }
    for(i = 0; i < task_dp_num; i++){
        char *t = task_dp[i];
        int *pair = ch_to_digit(t);
        int parent = pair[0]-1, child = pair[1]-1;
        task_data_dps[i][0] = parent;
        task_data_dps[i][1] = child;
        free(pair);
    }
}

void build_task_envir_dps(int task_envir_num, char **envir_dp, int **task_envir_dps)
{
    int *ch_to_digit(char *chs);
    int i;
    for(i = 0; i < task_envir_num; i++){
        int *temp = (int*)malloc(2 * sizeof(int));
        memset(temp, 0, 2 * sizeof(int));
        task_envir_dps[i] = temp;
    }
    for(i = 0; i < task_envir_num; i++){
        char *t = envir_dp[i];
        int *pair = ch_to_digit(t);
        int parent = pair[0]-1, child = pair[1]-1;
        task_envir_dps[i][0] = parent;
        task_envir_dps[i][1] = child;
        free(pair);
    }
}

int *ch_to_digit(char *chs)
{
    int i = 0, s = 0;
    int *ans = (int*)malloc(sizeof(int) * MAXNUM);
    int *q = ans;

    while (i < MAXNUM){
        if(chs[i] == '\0'){
            break;
        }
        if(chs[i] >= '0' && chs[i] <= '9'){
            s = 10*s + chs[i] - '0';
        }
        else{
            *q++ = s;
            s = 0;
        }
        i++;
    }
    *q = '\0';
    return ans;
}

struct List *createListNode(int data)
{
    struct List *newNode
        = (struct List*)malloc(sizeof(struct List));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}