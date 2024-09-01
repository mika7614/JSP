#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#define ROUND_DIV(a, b) ((a) / (b) + ((a) % (b) == 0 ? 0 : 1))
#define MAXNUM 1000
#define MAXTASK 10000

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
    int endTime;
    int machineID;
    int diskID;
    int parentMaxTime;
} Answer;

typedef struct controller{
    int *stack;
    int sp;
    int optVal;
    int curVal;
    int *disk_speeds;
    int *disk_quotas;
    int *newToOldDisks;
    int *coeffs;
    int *data_costs;
    int nonZeroDataNum;
    int *newToOldTasks;
    int *answer;
} Controller;

struct Graph {
    int V;
    int adj[MAXTASK][MAXTASK];
};

// Structure to represent a list (adjacency list)
struct List{
    int data;
    struct List *next;
};

int main()
{
    void read_raw_data(RawData *contents);
    void preprocess(RawData *contents, Variables *src);
    void output_variables(RawData contents, Variables src);
    Answer *solve_task_machine_map(RawData contents, Variables src, int *taskToDisk);
    int *solve_disk_assignment(RawData *contents, Variables *src);
    void clearAllVars(RawData contents, Variables *src, int *taskToDisk, Answer *ans);
    void outputAnswer(RawData contents, Answer *ans);

    freopen("input.txt", "r", stdin);

    RawData contents;
    read_raw_data(&contents);
    Variables src;
    preprocess(&contents, &src);

    fclose(stdin);

    int *taskToDisk = solve_disk_assignment(&contents, &src);
    Answer *ans = solve_task_machine_map(contents, src, taskToDisk);

    // output_variables(contents, src);
    freopen("output.txt", "w", stdout);
    outputAnswer(contents, ans);
    fclose(stdout);

    clearAllVars(contents, &src, taskToDisk, ans);
    return 0;
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

    for(int i = 0; i < contents.task_num; i++){
        for(int j = 0; j < src.task_machine_dp[i].nums; j++){
            printf("%d ", src.task_machine_dp[i].machines[j]);
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

    while(i < MAXNUM){
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

int *solve_disk_assignment(RawData *contents, Variables *src)
{
    void init_agent(Controller *agent, RawData *contents, Variables *src);
    bool backToParent(Controller *agent);
    void goToChild(Controller *agent);
    void updateState(Controller *agent);
    void undoState(Controller *agent);
    void saveAnswer(Controller *agent, RawData *contents);
    bool cmpOptVal(Controller agent);
    bool checkQuotas(Controller agent);
    int isLastTask(Controller agent);
    void clearAgent(Controller *agent);

    Controller *agent = (Controller*)malloc(sizeof(Controller));
    init_agent(agent, contents, src);

    while(agent->sp > -1){
        // printf("%d: %d\n", agent->sp, agent->stack[agent->sp]);
        if(cmpOptVal(*agent)){
            if(checkQuotas(*agent)){
                updateState(agent);
                if(isLastTask(*agent)){
                    saveAnswer(agent, contents);
                    undoState(agent);
                    if(!backToParent(agent)){
                        break;
                    }
                    continue;
                }
                goToChild(agent);
            }
            else{
                if(agent->stack[agent->sp] > contents->disk_num - 2){
                    if(!backToParent(agent)){
                        break;
                    }
                    continue;
                }
                agent->stack[agent->sp] += 1;
            }
        }
        else{
            if(!backToParent(agent)){
                break;
            }
        }
    }
    // Copy
    int *answer = (int*)malloc(sizeof(int) * contents->task_num);
    for(int i = 0 ; i < contents->task_num; i++){
        answer[i] = agent->answer[i];
    }
    clearAgent(agent);
    return answer;
}

void init_agent(Controller *agent, RawData *contents, Variables *src)
{
    int *genNewdiskID(RawData *contents, Variables *src);
    int *genCoeffs(RawData *contents, Variables *src);
    int *genNewTaskID(RawData *contents, Variables *src, int *coeffs);

    agent->stack = (int*)malloc(contents->task_num * sizeof(int));
    memset(agent->stack, 0, contents->task_num * sizeof(int));
    agent->sp = 0;
    agent->optVal = 0x7fffffff;
    agent->curVal = 0;

    int *disk_inds = genNewdiskID(contents, src);
    // Reorder disk_quotas and disk_speeds
    agent->disk_quotas = (int*)malloc(sizeof(int) * contents->disk_num);
    agent->disk_speeds = (int*)malloc(sizeof(int) * contents->disk_num);
    for(int i = 0; i < contents->disk_num; i++){
        agent->disk_quotas[i] = src->disk_quotas[disk_inds[i]];
        agent->disk_speeds[i] = src->disk_speeds[disk_inds[i]];
    }
    agent->newToOldDisks = disk_inds;

    agent->coeffs = genCoeffs(contents, src);
    int *task_inds = genNewTaskID(contents, src, agent->coeffs);
    // Reorder data_costs
    agent->data_costs = (int*)malloc(sizeof(int) * contents->task_num);
    memset(agent->data_costs, 0, contents->task_num * sizeof(int));
    agent->nonZeroDataNum = 0;
    for(int i = 0; i < contents->task_num; i++){
        int data_size = src->data_costs[task_inds[i]];
        if(data_size < 1){
            break;
        }
        agent->nonZeroDataNum += 1;
        agent->data_costs[i] = data_size;
    }
    // Reorder agent->coeffs
    int *tmpCoeffs = (int*)malloc(contents->task_num * sizeof(int));
    for(int i = 0; i < contents->task_num; i++){
        tmpCoeffs[i] = agent->coeffs[i];
    }
    for(int i = 0; i < contents->task_num; i++){
        agent->coeffs[i] = tmpCoeffs[task_inds[i]];
    }
    free(tmpCoeffs);
    agent->newToOldTasks = task_inds;
    agent->answer = (int*)malloc(sizeof(int) * contents->task_num);
}

bool backToParent(Controller *agent)
{
    if(agent->sp < 1){
        return false;
    }
    agent->stack[agent->sp] = 0;
    agent->sp -= 1;
    agent->stack[agent->sp] += 1;
    return true;
}

void goToChild(Controller *agent)
{
    agent->sp += 1;
    agent->stack[agent->sp] = 0;
}

void updateState(Controller *agent)
{
    int data_size = agent->data_costs[agent->sp];
    int newDiskID = agent->stack[agent->sp];
    agent->disk_quotas[newDiskID] -= data_size;

    int totalDataSize = agent->coeffs[agent->sp] * agent->data_costs[agent->sp];
    int increment = ROUND_DIV(totalDataSize, agent->disk_speeds[newDiskID]);
    agent->curVal += increment;
}

void undoState(Controller *agent)
{
    int data_size = agent->data_costs[agent->sp];
    int newDiskID = agent->stack[agent->sp];
    agent->disk_quotas[newDiskID] += data_size;

    int totalDataSize = agent->coeffs[agent->sp] * data_size;
    int increment = ROUND_DIV(totalDataSize, agent->disk_speeds[newDiskID]);
    agent->curVal -= increment;
}

bool cmpOptVal(Controller agent)
{
    int newDiskID = agent.stack[agent.sp];
    int data_size = agent.data_costs[agent.sp];
    int totalDataSize = agent.coeffs[agent.sp] * data_size;
    int newVal = agent.curVal + ROUND_DIV(totalDataSize, agent.disk_speeds[newDiskID]);
    if(newVal > agent.optVal){
        return false;
    }
    else{
        return true;
    }
}

bool checkQuotas(Controller agent)
{
    int newDiskID = agent.stack[agent.sp];
    int data_size = agent.data_costs[agent.sp];
    if(data_size > agent.disk_quotas[newDiskID]){
        return false;
    }
    else{
        return true;
    }
}

int isLastTask(Controller agent)
{
    if(agent.sp > agent.nonZeroDataNum - 2){
        return true;
    }
    else{
        return false;
    }
}

void saveAnswer(Controller *agent, RawData *contents){
    for(int i = 0; i < contents->task_num; i++){
        int task_id = agent->newToOldTasks[i];
        int newDiskID = agent->stack[i];
        int disk_id = agent->newToOldDisks[newDiskID];
        agent->answer[task_id] = disk_id;
        // printf("%d ", newDiskID);
    }
    // update optimal value
    agent->optVal = agent->curVal;

    // printf("solution:\n");
    // for(int i = 0; i < contents->task_num; i++){
    //     printf("%d ", agent->answer[i]+1);
    // }
    // putchar('\n');
}

int *genNewdiskID(RawData *contents, Variables *src)
{
    void mergeSort(int nums[], int indices[], int n);
    int *disk_inds = (int*)malloc(sizeof(int) * contents->disk_num);
    for(int i = 0; i < contents->disk_num; i++){
        disk_inds[i] = i;
    }
    int *tmpDiskSpeeds = (int*)malloc(contents->disk_num * sizeof(int));
    for(int i = 0; i < contents->disk_num; i++){
        tmpDiskSpeeds[i] = (src->disk_speeds)[i];
    }
    // Sort w.r.t. disk speed, from quick to slow
    mergeSort(tmpDiskSpeeds, disk_inds, contents->disk_num);
    free(tmpDiskSpeeds);
    return disk_inds;
}

int *genCoeffs(RawData *contents, Variables *src)
{
    int task_num = contents->task_num;
    int task_dp_num = contents->task_dp_num;
    int *coeffs = (int*)malloc(sizeof(int) * task_num);
    memset(coeffs, 0, task_num * sizeof(int));
    for(int i = 0; i < task_dp_num; i++){
        int parent = src->task_data_dps[i][0];
        coeffs[parent] += 1;
    }
    // Task need time to output its data
    for(int parent = 0; parent < task_num; parent++){
        if(src->data_costs[parent] < 1){
            continue;
        }
        coeffs[parent] += 1;
    }
    return coeffs;
}

int *genNewTaskID(RawData *contents, Variables *src, int *coeffs)
{
    void mergeSort(int nums[], int indices[], int n);
    int *task_inds = (int*)malloc(sizeof(int) * contents->task_num);
    for(int i = 0; i < contents->task_num; i++){
        task_inds[i] = i;
    }
    // Sort w.r.t. (data_size * coeffs), from large to small
    int *tmpDataCosts = (int*)malloc(contents->task_num * sizeof(int));
    for(int i = 0; i < contents->task_num; i++){
        tmpDataCosts[i] = (src->data_costs)[i] * coeffs[i];
    }
    mergeSort(tmpDataCosts, task_inds, contents->task_num);
    free(tmpDataCosts);
    return task_inds;
}

void clearAgent(Controller *agent)
{
    free(agent->stack);
    free(agent->disk_quotas);
    free(agent->disk_speeds);
    free(agent->newToOldDisks);
    free(agent->coeffs);
    free(agent->data_costs);
    free(agent->newToOldTasks);
    free(agent->answer);
    free(agent);
}

Answer *solve_task_machine_map(RawData contents, Variables src, int *taskToDisk)
{
    int *solve_task_order_ans(RawData contents, Variables src);
    int *randomMachineAssignment(RawData contents, Variables src);
    Answer *calcEndTime(int *taskToMachine, int *taskToDisk, int *taskOrder,
                        RawData contents, Variables src);
    
    int *taskOrder = solve_task_order_ans(contents, src);

    freopen("taskOrder.txt", "w", stdout);
    for(int i = 0; i < contents.task_num; i++){
        printf("%d ", taskOrder[i]+1);
    }
    fclose(stdout);
    freopen("CON", "w", stdout);

    int *taskToMachine = randomMachineAssignment(contents, src);

    Answer *ans = calcEndTime(taskToMachine, taskToDisk, taskOrder, contents, src);
    free(taskOrder);
    free(taskToMachine);
    return ans;
}

int *randomMachineAssignment(RawData contents, Variables src)
{
    int *result = (int*)malloc(contents.task_num * sizeof(int));
    srand((unsigned)time(NULL));
    for(int i = 0; i < contents.task_num; i++){
        int nums = src.task_machine_dp[i].nums;
        int *machines = src.task_machine_dp[i].machines;
        int index = rand() % nums;
        result[i] = machines[index];
    }
    return result;
}

Answer *calcEndTime(int *taskToMachine, int *taskToDisk, int *taskOrder,
                    RawData contents, Variables src)
{
    struct List **genChildParentMap(RawData contents, Variables src);
    int calcMaxParentTime(Answer *ans, int taskID,
                          struct List** childrenToParents);
    int *calcAllDataTime(RawData contents, Variables src, int *taskToDisk);
    int *calcAllTaskTime(RawData contents, Variables src, int *taskToMachine);
    void clearChildrenToParents(struct List **childrenToParents, RawData contents);

    struct List **childrenToParents = genChildParentMap(contents, src);
    Answer *ans = (Answer*)malloc(contents.task_num * sizeof(Answer));

    for(int i = 0; i < contents.task_num; i++){
        ans[i].startTime = 0;
        ans[i].endTime = 0;
    }
    int *allDataTime = calcAllDataTime(contents, src, taskToDisk);
    int *allTaskTime = calcAllTaskTime(contents, src, taskToMachine);
    int *diskTimes = (int*)malloc(contents.task_num * sizeof(int));
    for(int i = 0; i < contents.task_num; i++){
        diskTimes[i] = allDataTime[i];
    }
    for(int i = 0; i < contents.task_dp_num; i++){
        int parent = src.task_data_dps[i][0];
        int child = src.task_data_dps[i][1];
        diskTimes[child] += allDataTime[parent];
    }
    int *machineEndTime = (int*)malloc(contents.ms_num * sizeof(int));
    memset(machineEndTime, 0, contents.ms_num * sizeof(int));
    for(int i = 0; i < contents.task_num; i++){
        int taskID = taskOrder[i];
        int machineID = taskToMachine[taskID];
        int diskID = taskToDisk[taskID];
        ans[taskID].machineID = machineID;
        ans[taskID].diskID = diskID;
        int curMachineEndTime = machineEndTime[machineID];
        int maxParentTime = calcMaxParentTime(ans, taskID, childrenToParents);
        ans[taskID].startTime = (curMachineEndTime > maxParentTime) ? curMachineEndTime : maxParentTime;
        int taskCost = allTaskTime[taskID] + diskTimes[taskID];
        ans[taskID].endTime = ans[taskID].startTime + taskCost;
        machineEndTime[machineID] = ans[taskID].endTime;
    }
    clearChildrenToParents(childrenToParents, contents);
    free(childrenToParents);
    free(allDataTime);
    free(allTaskTime);
    free(diskTimes);
    free(machineEndTime);
    return ans;
}

int *calcAllDataTime(RawData contents, Variables src, int *taskToDisk)
{
    int *allDataTime = (int*)malloc(contents.task_num * sizeof(int));
    for(int i = 0; i < contents.task_num; i++){
        int dataSize = src.data_costs[i];
        int disk_id = taskToDisk[i];
        int disk_speed = src.disk_speeds[disk_id];
        allDataTime[i] = ROUND_DIV(dataSize, disk_speed);
    }
    return allDataTime;
}

int *calcAllTaskTime(RawData contents, Variables src, int *taskToMachine)
{
    int *allTaskTime = (int*)malloc(contents.task_num * sizeof(int));
    for(int i = 0; i < contents.task_num; i++){
        int taskSize = src.task_costs[i];
        int machineID = taskToMachine[i];
        int power = src.ms_power[machineID];
        allTaskTime[i] = ROUND_DIV(taskSize, power);
    }
    return allTaskTime;
}

struct List **genChildParentMap(RawData contents, Variables src)
{
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
    for(int i = 0; i < contents.task_envir_num; i++){
        int parentID = src.task_envir_dps[i][0];
        int childID = src.task_envir_dps[i][1];
        struct List *newNode = createListNode(parentID);
        newNode->next = childrenToParents[childID];
        childrenToParents[childID] = newNode;
    }
    return childrenToParents;
}

int calcMaxParentTime(Answer *ans, int taskID,
                      struct List** childrenToParents)
{
    int maxEndTime = 0;
    struct List *parentNode = childrenToParents[taskID];

    while(parentNode != NULL){
        int parentID = parentNode->data;
        int endTime = ans[parentID].endTime;
        if(maxEndTime < endTime){
            maxEndTime = endTime;
        }
        parentNode = parentNode->next;
    }
    return maxEndTime;
}

int *solve_task_order_ans(RawData contents, Variables src)
{
    int *kahnTopologicalSort(struct Graph *graph, int task_num);
    struct Graph *createGraph(int V);
    void addEdge(struct Graph *graph, int u, int v);

    struct Graph *graph = createGraph(contents.task_num);
    for(int i = 0; i < contents.task_dp_num; i++){
        int parent = src.task_data_dps[i][0];
        int child = src.task_data_dps[i][1];
        addEdge(graph, parent, child);
    }
    for(int i = 0; i < contents.task_envir_num; i++){
        int parent = src.task_envir_dps[i][0];
        int child = src.task_envir_dps[i][1];
        addEdge(graph, parent, child);
    }
    int *task_order_ans = (int*)malloc(contents.task_num * sizeof(int));

    int *ans = kahnTopologicalSort(graph, contents.task_num);
    for(int i = 0; i < contents.task_num; i++){
        task_order_ans[i] = ans[i];
    }
    free(graph);
    free(ans);
    return task_order_ans;
}

void clearAllVars(RawData contents, Variables *src, int *taskToDisk, Answer *ans)
{
    free(src->task_costs);
    free(src->data_costs);
    for(int i = 0; i < contents.task_num; i++){
        free(src->task_machine_dp[i].machines);
    }
    free(src->task_machine_dp);
    free(src->ms_power);
    free(src->disk_speeds);
    free(src->disk_quotas);
    for(int i = 0; i < contents.task_dp_num; i++){
        free(src->task_data_dps[i]);
    }
    free(src->task_data_dps);
    for(int i = 0; i < contents.task_envir_num; i++){
        free(src->task_envir_dps[i]);
    }
    free(src->task_envir_dps);
    free(taskToDisk);
    free(ans);
}

void clearChildrenToParents(struct List **childrenToParents, RawData contents)
{
    for(int i = 0; i < contents.task_num; i++){
        struct List *node = childrenToParents[i];
        struct List *prevNode;
        while(node != NULL){
            prevNode = node;
            node = node->next;
            free(prevNode);
        }
    }
}

void outputAnswer(RawData contents, Answer *ans)
{
    for(int i = 0; i < contents.task_num; i++){
        int taskID = i;
        int startTime = ans[i].startTime;
        int machineID = ans[i].machineID;
        int diskID = ans[i].diskID;
        printf("%d %d %d %d\n", taskID+1, startTime, machineID+1, diskID+1);
        // int endTime = ans[i].endTime;
        // printf("%d %d %d %d %d\n", taskID+1, startTime, machineID+1, diskID+1, endTime);
    }
}

//############################### Merge Sort Algorithm ###############################//
void mergeSort(int nums[], int indices[], int n)
{
    void mSort(int nums[], int indices[], int tmpArray[], int left, int right);
    int *tmpArray;

    tmpArray = (int*)malloc(n * sizeof(int));
    if(tmpArray != NULL){
        mSort(nums, indices, tmpArray, 0, n-1);
        free(tmpArray);
    }
    else{
        printf("No space for tmp array!!!");
    }
}

void mSort(int nums[], int indices[], int tmpArray[], int left, int right)
{
    void merge(int nums[], int indices[], int tmpArray[],
               int left, int right, int rightEnd);
    int center;

    if(left < right){
        center = (left + right) / 2;
        mSort(nums, indices, tmpArray, left, center);
        mSort(nums, indices, tmpArray, center+1, right);
        merge(nums, indices, tmpArray, left, center+1, right);
    }
}

void merge(int nums[], int indices[], int tmpArray[],
           int left, int right, int rightEnd)
{
    int i, leftEnd, numElements, tmpPos;

    leftEnd = right - 1;
    tmpPos = left;
    numElements = rightEnd - left + 1;

    while(left <= leftEnd && right <= rightEnd){
        if(nums[indices[left]] > nums[indices[right]]){
            tmpArray[tmpPos++] = indices[left++];
        }
        else{
            tmpArray[tmpPos++] = indices[right++];
        }
    }

    while(left <= leftEnd){
        tmpArray[tmpPos++] = indices[left++];
    }
    while(right <= rightEnd){
        tmpArray[tmpPos++] = indices[right++];
    }

    for(i = 0; i < numElements; i++, rightEnd--){
        indices[rightEnd] = tmpArray[rightEnd];
    }
}

//############################### Topological Sort Algorithm ###############################//
// Create a new node for the stack
struct List *createListNode(int data)
{
    struct List *newNode
        = (struct List*)malloc(sizeof(struct List));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

struct Graph *createGraph(int V)
{
    struct Graph *graph
        = (struct Graph*)malloc(sizeof(struct Graph));
    graph->V = V;

    for(int i = 0; i < V; i++)
        for(int j = 0; j < V; j++)
            graph->adj[i][j] = 0;

    return graph;
}

void addEdge(struct Graph *graph, int u, int v)
{
    graph->adj[u][v] = 1;
}

int *kahnTopologicalSort(struct Graph *graph, int task_num)
{
    int in_degree[MAXTASK] = {0};

    for(int i = 0; i < graph->V; i++)
        for(int j = 0; j < graph->V; j++)
            if(graph->adj[i][j])
                in_degree[j]++;

    int queue[MAXTASK], front = 0, rear = 0;
    for(int i = 0; i < graph->V; i++)
        if(in_degree[i] == 0)
            queue[rear++] = i;

    int count = 0;
    int *top_order = (int*)malloc(task_num * sizeof(int));

    while(front < rear){
        int u = queue[front++];
        top_order[count++] = u;

        for(int i = 0; i < graph->V; i++) {
            if(graph->adj[u][i]){
                if(--in_degree[i] == 0)
                    queue[rear++] = i;
            }
        }
    }
    return top_order;
}
