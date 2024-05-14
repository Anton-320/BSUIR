#include "fun.h"

PAIR pair = {0, 0};
int makeValues = 1;
int zeroZeroCount = 0;
int zeroOneCount = 0;
int oneZeroCount = 0;
int oneOneCount = 0;
int canShow = 1;

void makeFork() {
    pid_t chpid = fork();                                      // создать процесс
    if(chpid == 0) {
        signal(SIGALRM, takeStatistics);                       // обработчик сигнала будильника
        cycle();
    } else if (chpid == -1) {
        printf("Error occured, error code - %d\n", errno);     // ошибка
        exit(errno);
    } else {
        push(&head, chpid);                                    // сохранение ид процесса.
        printf("Process with pid = %d created.\n", chpid);
    }
}

void delLastFork() {
    kill(head->pid, SIGKILL);                                  // Сигнал завершения процессу.
    printf("Process C_%d with pid = %d deleted.\n", head->num, head->pid);
    pop(&head);
}

void forbid() {
    canShow = 0;
}

void allow() {
    canShow = 1;
}

void sendForbid(int procNum) {
    if (procNum == -1) {
        NODE *temp = head;
        while (temp) {
            kill(temp->pid, SIGUSR2);                          // Сигнал запрщения вывода всем.
            temp = temp->next;
        }
    } else {
        NODE *temp = head;
        while (temp && temp->pid != procNum)
            temp = temp->next;
        if (temp)
            kill(temp->pid, SIGUSR2);                          // Сигнал запрещения вывода одному.
    }
}

void sendAllow(int procNum) {
    if (procNum == -1) {
        NODE *temp = head;
        while (temp) {
            kill(temp->pid, SIGUSR1);                          // Сигнал разрешения вывода всем.
            temp = temp->next;
        }
    } else {
        NODE *temp = head;
        while (temp && temp->pid != procNum)
            temp = temp->next;
        if (temp)
            kill(temp->pid, SIGUSR1);                          // Сигнал разрешения вывода одному.
    }
}

void forbidAndShow(int procNum) { 
    sendForbid(-1);                                            // Запретить всем.
    NODE *temp = head;
    while (temp && temp->pid != procNum)
        temp = temp->next;
    if (temp)
        kill(temp->pid, SIGINT);                               // Вывести данные указанного процесса.
    signal(SIGALRM, allowAfterP);
    alarm(5);                                                  // Таймер чтобы вернуть всё назад.
}

void printStat() {
    printf("Process pid = %d, ppid = %d, {0,0} - %d, {0,1} - %d, {1,0} - %d, {1,1} - %d\n",
           getpid(), getppid(), zeroZeroCount, zeroOneCount, oneZeroCount, oneOneCount);
}

void cycle() {
    int currentRepeat = 0;
    while(1) {
        if(currentRepeat == REPEAT_COUNT) {
            currentRepeat = 0;
            if (canShow)
                printStat();
        }
        alarm(1);
        while (makeValues == 1) {
            if (pair.firstVal == 0 && pair.secondVal == 0)
                pair.secondVal = 1;
            else if (pair.firstVal == 0 && pair.secondVal == 1) {
                pair.firstVal = 1;
                pair.secondVal = 0;
            } else if (pair.firstVal == 1 && pair.secondVal == 0)
                pair.secondVal = 1;
            else {
                pair.firstVal = 0;
                pair.secondVal = 0;
            }

        }
        makeValues = 1;
        currentRepeat++;
    }
}

void takeStatistics() {
    if (pair.firstVal == 0 && pair.secondVal == 0)
        zeroZeroCount++;
    else if (pair.firstVal == 0 && pair.secondVal == 1)
        zeroOneCount++;
    else if (pair.firstVal == 1 && pair.secondVal == 0)
        oneZeroCount++;
    else
        oneOneCount++;
    makeValues = 0;
}

void allowAfterP() {
    sendAllow(-1);
}