//Адресное пространство с одним или несколькими потоками, выполняющимися в этом адресном
//пространстве, и необходимые системные ресурсы для этих потоков.


#include "fun.h"

NODE* head = NULL;

int main() {
    signal(SIGINT, printStat);                                                  // Сигнал вывода информации.
    signal(SIGUSR1, allow);                                                     // Сигнал разрешения вывода данных.
    signal(SIGUSR2, forbid);                                                    // Сигнал запрещения вывода данных.

    int cont = 1;
    while(cont) {
        char ch = getchar(), n;
        int num = -1;
        while((n=getchar())!='\n') {
            if (num == -1) num++;
            num *= 10;
            num += n - '0';
        }
        switch(ch) {
            case '+':
                makeFork();                                                    // создать процесс
                break;
            case '-':
                delLastFork();                                                 // удалить процесс
                break;
            case 'l':
                printf("Process P with pid = %d\n", getpid());                 // вывести все процессы
                show(head);
                break;
            case 'k':
                while(head)
                    delLastFork();                                             // удалить все
                break;
            case 's':
                sendForbid(num);                                               // запретить
                printf("Output is forbidden\n");
                break;
            case 'g':
                sendAllow(num);                                                // разрешить
                printf("Output is allowed\n");
                break;
            case 'p':
                forbidAndShow(num);                                            // вывести данные одним процессом, запретив другим
                break;
            case 'q':
                while(head)
                    delLastFork();                                             // конец
                printf("All processes are deleted. Program ends.\n");
                cont = 0;
                break;
        }
    }

    return 0;
}