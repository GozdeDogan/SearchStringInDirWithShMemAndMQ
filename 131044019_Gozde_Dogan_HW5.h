////////////////////////////////////////////////////////////////////////////////
// Gozde DOGAN 131044019
// Homework 5
// h dosyasi
// 
// Description:
//      Girilen directory icerisindeki her file'da yine girilen stringi aradim.
//      String sayisini ekrana yazdirdim
//      Her buldugum string'in satir ve sutun numarasini buldugum, 
//      process idsi ve thread idsi ile birlikte log.txt dosyasina yazdim.
//      Her file dan sonra o file da kac tane string oldugunu da 
//      log dosyasina yazdim.
//      Her directory icin process(fork olusturuldu) olusturuldu. 
//      Directoryler icindeki her file icin de thread olusturulur.
//      Islem yapilirken directory-directory haberlesmesi shared memory, 
//      file-directory haberlesmesi message queue ile saglandi.
//
// Ekran Ciktisi:
//      1. bulunan toplam string sayisi
//      2. toplam directory sayisi (parametre olarak girilen directory dahil)
//      3. toplam file sayisi
//      4. toplam line sayisi
//      5. olusturulan process sayisi
//      6. olusturulan shared memory sayisi
//      7. olusturulan toplam thread sayisi
//      8. arama isleminin tamamlanma suresi
//      9. cikis nedeni
//
// Important:
//      Kitaptaki restart.h kutuphanesi kullaildi.
//      restart.h ve restart.c dosyalari homework directory'sine eklendi.
//      istenilen degerlerin bulunabilmesi icin temp dosyalari olusturuldu
//      islemler bitince dosyalar da kaldirildi.
//      log.txt ise log file!
//
// References:
//      1. DirWalk fonksiyonunun calisma sekli icin bu siteden yararlanildi.
//      https://gist.github.com/semihozkoroglu/737691
//      
//      2. thread fonksiyonlarinin calisma sekli icin.
//      https://computing.llnl.gov/tutorials/pthreads/
//      
//      3. semaphore fonksiyonlarinin kullanimi bu siteden incelendi.
//      http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
//      
//      4. sinyal incelemesi
//      https://www.tutorialspoint.com/c_standard_library/c_function_signal.htm
//         
//      5. time hesabi
//      https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/qnx/clock_gettime.html
//
//      6. shared memory
//      https://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
//      http://stackoverflow.com/questions/5656530/how-to-use-shared-memory-with-linux-in-c
//      
//      7. message queue
//      http://stackoverflow.com/questions/22661614/message-queue-in-c-implementing-2-way-comm
//      https://users.cs.cf.ac.uk/Dave.Marshall/C/node25.html
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// LIBRARIES ////////////////////////////////////
#include <stdio.h>
#include <stdlib.h> 
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include "restart.h"
////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////// MACROS ////////////////////////////////////// 
#define MAXPATHSIZE 1024
#define DEBUG
#define MAXSIZE 100000
#define SHMSIZE 9999

#define SIZE 100

#define LOGFILE "log.txt"
#define SIZEBUFFER 1000
#define bufferSizeUp 1001
#define fifoName "gozde"

#define KEYMQ 906578318
////////////////////////////////////////////////////////////////////////////////

///////////////////////////// STRUCT YAPILARI //////////////////////////////////
typedef struct{
    char fname[MAXPATHSIZE];
    int pData;
    pid_t processID;
    pid_t threadID;
    int msqid;
} threadData;

typedef struct {
    int iNumOfWords;
    int iNumOfDirectories;
    int iNumOfFiles;
    int iNumOfLines;
    int iNumOfProcess;
    int iNumOfCascadeThreads;
    int iNumOfThreads;
    int iNumOfMaxThreads;
    double iTimeOperations;
} numOf;

typedef struct {
    int value;
    char msg[SIZE];
} message;
///////////////////////////////////////////////////////////////////////////////


//////////////////////////// Function prototypes ///////////////////////////////
int searchStringInFile(char *sFileName, int pipeFd, pid_t pID, pid_t tID);
//Genel islemlerimi topladigim bir fonksiyon

int isEmpty(FILE *file);
//Gelen dosyanin bos olup olmadigina bakar

char** readToFile(); 
//Dosyayi okuyup iki boyutlu stringe yazacak

void findLengthLineAndNumOFline();
//Dosyadaki satir sayisini ve en uzun satirin sütün sayisini hesapliyor

int searchString(char* sFileName, char **sFile, int pipeFd, pid_t pID, pid_t tID);
//string iki boyutlu string icinde arayacak

int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol);
//1 return ederse kopyalama yapti, 0 return ederse kopyalama yapamadi

int DirWalk(const char *path, sem_t *semp);
//fork yaparak her directory'nin icine girer, thread ile de her file icine girer

void *threadOperations(void * dataOfThread);
//thread icin cagirilan fonksiyon

void openFiles(char path[MAXPATHSIZE]);
//ekrana yazilmasi istenilen degerlerin tutuldugu temp dosyalarini acar

void calculateNumOfValues();
//istenilen degerlerin dosyadan gerektigi gibi okunup hesaplanma islemi gerceklestirilir

void closeFiles();
//temp dosyalarini kapatir

void signalHandle(int sig);
//sinyalleri yakalar ve gerektigi sekilde cikis yapilmasini saglar
///////////////////////////////////////////////////////////////////////////////
