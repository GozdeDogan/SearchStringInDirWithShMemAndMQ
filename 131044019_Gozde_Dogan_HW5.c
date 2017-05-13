////////////////////////////////////////////////////////////////////////////////
// Gozde DOGAN 131044019
// Homework 5
// c dosyasi
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

/////////////////////////////////// LIBRARY ////////////////////////////////////
#include "131044019_Gozde_Dogan_HW5.h"

////////////////////// Global Variables ////////////////////////////////////////
int iNumOfLine = 0;
int iLengthLine = 0;

FILE *fPtrInFile = NULL;

char *sSearchStr = NULL;
int iSizeOfSearchStr = 0;

FILE *fPtrDirs;
FILE *fPtrProcess;
FILE *fPtrWords;
FILE *fPtrFiles;
FILE *fPtrLines;;
FILE *fPtrThreads;

char pathWords[MAXPATHSIZE], pathDirs[MAXPATHSIZE], pathProcess[MAXPATHSIZE], pathFiles[MAXPATHSIZE], pathLines[MAXPATHSIZE], pathThreads[MAXPATHSIZE];

threadData dataOfThread[MAXSIZE];
pthread_t threadArr[MAXSIZE];
int sizeOfThread = 0;

numOf numOfValues;
int iDirs = 0;
int iMaxThreads = 0;
int logFifo = 0; //log dosyasi


char *data;
int counts = 0;
int out;
char *buffertemp;
char allpath[MAXSIZE];
char *tempChar;
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]){ 
    /////////////////////////////// VARIABLES //////////////////////////////////   
    char s[MAXSIZE]; // fifoya yazialacak seyleri bu string'e atip yazdim
    int i=0; // tempFile.txt'den okuma yapabilmek icin kullandim
    char path[MAXPATHSIZE];
	
	struct timespec n_start, n_stop; //zaman hesabi icin kullanilan degiskenler
	
	key_t key;
    int shmid;   
    ////////////////////////////////////////////////////////////////////////////
   
    
    if (argc != 3) {
        printf("\n*******************************************************\n");
        printf ("\n    Usage>> ");
        printf("./grepTh \"searchString\" <directoryName>\n\n");
        printf("*******************************************************\n\n");
        return 1;
    }
   
    if ((key = ftok("131044019_Gozde_Dogan_HW5.c", 'R')) == -1) {
		    perror("ftok");
		    exit(1);
	}
		
		
	if ((shmid = shmget(key, SHMSIZE, 0644 | IPC_CREAT)) == -1) {
		    perror("shmget");
		    exit(1);
	}

	data = shmat(shmid, (void *)0, 0);
	
	
	if (data == (char *)(-1)) {
			perror("shmat");
		    exit(1);
	}
   

    iSizeOfSearchStr = strlen(argv[1]);
    if(argv[1] != NULL)
        sSearchStr = (char*)calloc((strlen(argv[1])+1), sizeof(char));
    strncpy(sSearchStr, argv[1], ((int)strlen(argv[1])+1));
    
    //temp dosyalarinin kaldirilabilmesi icin bulunduklari path hesaplandi
    getcwd(path, MAXPATHSIZE);
    //temp dosyalari olusturulup, acildi
    openFiles(path);
    
    ////////////////////////////////////////////////////////////////////////////
    sprintf(allpath, "%s/temp.txt", path);
	out = open(allpath,  O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ////////////////////////////////////////////////////////////////////////////
    
    // semaphore olusturuldu
    sem_t semaphore;	
	sem_init(&semaphore, 0, 1);   
	
	//log dosyasi
    logFifo = open(LOGFILE, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    
    //islemin hesaplanmaya baslanildigi zaman hesaplandi
    if( clock_gettime( CLOCK_REALTIME, &n_start) == -1 ) {
      perror( "clock gettime" );
      return EXIT_FAILURE;
    }
    
    //directory ler arasi gezinme ve diger islemlerinin gerceklestirildigi fonksiyon
    numOfValues.iNumOfWords = DirWalk(argv[2], &semaphore);
    
    //semaphore'un isleminin bitmesi beklendi
    //semaphore islemi bitene kadar kritik bolgeye girmesi engellendi
    sem_wait(&semaphore);
    
    int k=0;
	while(data[k]!= '\0')
	++k;
	
	/*shared memoryden alinan bilgi gfd'ye yaziliyor*/		
	r_write(logFifo, data, k);
	
	//olusturulan ana fifonun silinmesi icin gerekti!
	chdir("..");
	
	//semaphore'un islemi sonlandirildi
	sem_post(&semaphore);

    //gerekli degrler hesaplandi
    calculateNumOfValues();       
        
    //islemin bittigi zaman bulundu.
    if( clock_gettime( CLOCK_REALTIME, &n_stop) == -1 ) {
      perror( "clock gettime" );
      return EXIT_FAILURE;
    }
    
    //islemin gerceklesme suresi hesaplandi
    numOfValues.iTimeOperations = ((n_stop.tv_sec - n_start.tv_sec) *1000 / CLOCKS_PER_SEC) + ( n_stop.tv_nsec - n_start.tv_nsec );
    
    //acilan temp dosyalari kapatildi ve kaldirildi
    closeFiles();
    
    ///////////////////////////// print screen ////////////////////////////////
    printf("\n*******************************************************\n");
    printf("  Total number of strings found : %d\n", numOfValues.iNumOfWords);
    printf("  Number of directories searched: %d\n", numOfValues.iNumOfDirectories);
    printf("  Number of files searched: %d\n", numOfValues.iNumOfFiles);
    printf("  Number of lines searched: %d\n", numOfValues.iNumOfLines);
    printf("  Number of process created: %d\n", numOfValues.iNumOfProcess);
    printf("  Number of cascade threads created: %d\n", numOfValues.iNumOfCascadeThreads);
    printf("  Number of search threads created: %d\n", numOfValues.iNumOfThreads);
    printf("  Number of shared memory created: %d\n", numOfValues.iNumOfDirectories);
    printf("  Max # of threads running concurrently: %d\n", numOfValues.iNumOfMaxThreads);
    printf("  Total run time, in milliseconds: %.4f\n", numOfValues.iTimeOperations);
    printf("  Exit Condition: normal\n");
    printf("*******************************************************\n\n"); 

    
    /////////////////////////////// print log //////////////////////////////////
    sprintf(s, "\n*******************************************************\n");
    write(logFifo, s, strlen(s));
    sprintf(s, "  Total number of strings found : %d\n", numOfValues.iNumOfWords);
    write(logFifo, s, strlen(s));    
    sprintf(s, "  Number of directories searched: %d\n", numOfValues.iNumOfDirectories);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of files searched: %d\n", numOfValues.iNumOfFiles);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of lines searched: %d\n", numOfValues.iNumOfLines);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of process created: %d\n", numOfValues.iNumOfProcess);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of cascade threads created: %d\n", numOfValues.iNumOfCascadeThreads);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of search threads created: %d\n", numOfValues.iNumOfThreads);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Number of shared memory created: %d\n", numOfValues.iNumOfDirectories);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Max # of threads running concurrently: %d\n", numOfValues.iNumOfMaxThreads);
    write(logFifo, s, strlen(s));
    sprintf(s, "  Total run time, in milliseconds: %.4f\n", numOfValues.iTimeOperations);
    write(logFifo, s, strlen(s)); 
    sprintf(s, "  Exit Condition: normal\n");
    write(logFifo, s, strlen(s)); 
    sprintf(s, "*******************************************************\n\n");
    write(logFifo, s, strlen(s));
    
    free(sSearchStr);
    close(logFifo);
    
	shmdt (data); 
 	shmctl (shmid, IPC_RMID, 0); 
 	
    return 0;
}


///////////////////////////// Function Definitions /////////////////////////////

/**
 * Verilen directory icine girer.
 * directory icindeki her dosya icinde istenilen stringi arar.
 * Her directroy icin fork olusturur.
 * Her file icin thread olsuturur.
 * semaphore kullanildi!
 *
 * sNameDir: aranacak directory
 * fifo: kullanilan main fifo
 * semp: file'lar icin olusturulan semaphore(threadler)
 */
int DirWalk(const char *sNameDir, sem_t *semp){    
    //////////////// variables /////////////////
    DIR *dir;
    struct dirent *mdirent = NULL; //recursive degisen directory
    struct stat buf; //file ve dir bilgisi ogrenmek icin

    pid_t pid; //forktan donen deger
    
    char fname[MAXPATHSIZE];
    char nameDirFile[MAXPATHSIZE];
    
    char path[MAXPATHSIZE]; //icinde bulunulan path
    char tempPath[MAXPATHSIZE]; //icinde bulunulan pathi olusturmak icin kullanilan temp path
    
    int **arrp; //p array
    int iSizep = 0;
    
    int iCount = 0; //bir file daki string sayisini tutar
    int iWords = 0; //toplam word sayisi
    int i = 0;
    int size = 0;
    int iThreads = 0; //olusturulan thread sayisini hesaplamak icin tutulan degisken
    int iTh = 0;
    char s[MAXSIZE]; //fifoya(loga) yazmak icin kullanilan string
    
    pthread_t myThread;
    
    int msqid;
    message *rcvmsg;
    ////////////////////////////////////////////////
    
    //p arrayleri icin yer ayrildi
    arrp = calloc(0, 0);

    getcwd(tempPath, MAXPATHSIZE);
    sprintf(path, "%s/%s", tempPath, sNameDir);
    
    chdir(path); 
    //path bufa atilip kontrol edildi
    /*path bufa atilip kontrol ediliyor*/
    if (stat(path, &buf) == -1)
        return -1;

    /* directory test ediliyor */
    if (S_ISDIR(buf.st_mode)) {

        if ((dir = opendir(path)) == NULL) { /*directory acilmiyorsa*/
            printf("%s can't open.\n", path);
            exit(EXIT_FAILURE);
        }
        else {

            /*butun directoryleri dolasip regular dosyalarda kelimemizi aratiyoruz*/
            while ((mdirent = readdir(dir)) != NULL) {

                if (strcmp(mdirent->d_name, ".") != 0 && strcmp(mdirent->d_name, "..") != 0 && mdirent->d_name[strlen(mdirent->d_name) - 1] != '~') {

                    sprintf(nameDirFile, "%s/%s", path, mdirent->d_name);

                    if (stat(nameDirFile, &buf) == -1)
                        return -1;
					
                    if (S_ISDIR(buf.st_mode)) {
                        fprintf(fPtrDirs, "1\n");
                
                        pid = fork();

                        if(pid == -1){
                             perror("Failed to fork\n");
                             printf("  Exit Condition: (cant fork) due to error %d\n", EXIT_FAILURE);
                             sprintf(s, "  Exit Condition: (cant fork) due to error %d\n", EXIT_FAILURE);
                             write(logFifo, s, strlen(s));   
                             closedir(dir);
                             return 0;
                        }

                        if (pid == 0) { /*cocuksa*/
                            fprintf(fPtrProcess, "1\n");
                            /*cocuksa DirWalk fonskiyonunu tekrar cagiriyoruz*/
                            DirWalk(mdirent -> d_name, semp);

                            closedir(dir);
                            exit(0);
                        } 
                    }
                  
                   
                   /*file ise thread olusturuyoruz*/
                    else if (S_ISREG(buf.st_mode)) {
                        // burada message queue olustur, send yap, thread fonksiyonunda da receive yapıp işini bitir.
                        iSizep++;
                        arrp = realloc(arrp, iSizep * sizeof (int*));
                        arrp[iSizep - 1] = calloc(2, sizeof (int));
                        pipe(arrp[iSizep - 1]);

                        ////////////////////////////////////////////////////////
                        // message queue olusturuldu.
                        msqid = msgget(KEYMQ, IPC_CREAT | 0666);
                        if (msqid < 0){
                            perror("msgget");
                            exit(1);
                        }
                        ////////////////////////////////////////////////////////
                        
                        //fname'e arastirilacak dosya adi yazildi
                        sprintf(fname, "%s", mdirent->d_name);
                        //arastirilacak dosya adi thread'e yollanilacak structure yapisina assign edildi
                        sprintf(dataOfThread[sizeOfThread].fname, "%s", fname);
                        //thread'e yollanilacak structure yapisina p assign edildi, bu p gerekli deger yazilacak
                        dataOfThread[sizeOfThread].pData = arrp[iSizep - 1][1];
                        //arastirma isleminin gerceklestirdigi process'in IDsi bulundu
                        dataOfThread[sizeOfThread].processID = getppid();                        
                        //message queue'nun id si thread fonksiyonu icinde kullanbilmesi icin thread datasina kopyalandi
                        dataOfThread[sizeOfThread].msqid = msqid;
                 
                 
                        iThreads++;
                        //fname icin thread olusturuldu
                        pthread_create(&threadArr[sizeOfThread], NULL, threadOperations, (void *)&dataOfThread[sizeOfThread]);
                        iTh++;
                        //printf("iTh:%d\n", iTh);
                        ++sizeOfThread;       
                        
                        ////////////////////////////////////////////////////////
                        //thread fonksiyonundan gonderilen mesaj alindi.
                        //Asagidaki yorum icindeki kisimla acilinca calismiyor. Bu nedenle yorum icinde.
                        /*if (msgrcv(msqid, &rcvmsg, SIZE, 1, 0) < 0){
                            perror("msgrcv");
                            exit(1);
                        }*/
                        ////////////////////////////////////////////////////////
                    }
                }
            }
        }
        while (r_wait(NULL) > 0); /*parent cocuklari bekliyor*/

        for (i = 0; i < sizeOfThread; ++i) 
            pthread_join(threadArr[i], NULL);
                
        //thread sayisi temp dosyasina yazildi 
        fprintf(fPtrThreads, "%d\n", iThreads);
        
        if(iTh >= iMaxThreads){
            iMaxThreads = iTh;
            iTh = 0;
            //printf("max:%d\n", iMaxThreads);
        }

/////////////////////////////////////////////////////////////////////////////////////////////
        int out2;/*file descriptor*/
        
        sem_wait(semp);
          
        /*p'tan gelenler shared memorye yaziliyor*/
        for (i = 0; i < iSizep; ++i) {
          
            close(arrp[i][1]);
            copyfile(arrp[i][0], out);
            
        	out2 = open(allpath,  O_RDONLY);
			while(read(out2, &tempChar, sizeof(char)) != 0)
				++counts;	
			close(out2);

			out2 = open(allpath,  O_RDONLY);
		 
			buffertemp = (char*) calloc(counts, (sizeof(char)));
			r_read(out2, buffertemp, counts);
	
        	memcpy(data, buffertemp, counts);
       		free(buffertemp);
        	close(out2);
        }
//////////////////////////////////////////////////////////////////////////////////
	
   		sem_post(semp);
    
    	counts=0;
        
        //plar temizlendi
        for (i = 0; i < iSizep; ++i)
            free(arrp[i]);
        free(arrp);  
    }
    closedir(dir);
    
    //Her sinyalin gelme olasiligi oldugu icin her sinyal icin yakalama islemi gerceklestirildi
    signal(SIGINT, signalHandle);
    signal(SIGTERM, signalHandle);
    signal(SIGILL, signalHandle);
    signal(SIGABRT, signalHandle);
    signal(SIGFPE, signalHandle);
    signal(SIGSEGV, signalHandle);
    
    return iWords;
}

/**
 * thread kullanimi icin yazildi
 * her olusturulan thread icin bu fonksiyon calsitirilir.
 * threadler her file icin olusturulur, boylece her file daki string sayisi bulunur.
 *
 * dataOfThread: 
 *          thread icin gelen parametre.
 *          thread icinde kullanilan fonksiyonun parametreleri!
 */
void *threadOperations(void * dataOfThread){  
    threadData *currentThreadData;
    
    ///////////////////////////
   // sndmsg'a bir seyler yazmak istedigimde hata veriyor.
    message *sndmsg;
    char s[MAXSIZE];
    size_t length;
    ///////////////////////////
    
    currentThreadData = (threadData *) dataOfThread;

    currentThreadData->threadID = getpid();

    iNumOfLine = 0;
    // dosyadaki string sayisi bulunur ve temp dosyasina yazilir.
    searchStringInFile(currentThreadData->fname, currentThreadData->pData, currentThreadData->processID, currentThreadData->threadID);

    fprintf(fPtrLines, "%d\n", iNumOfLine);
    //printf("%d\n", iNumOfLine); 

    int iFiles=0;
    iFiles++;
    fprintf(fPtrFiles, "%d\n", iFiles);
    
   /////////////////////////////////////////////////////////////////////////////
   // message queue ya mesaj yollandi. 
    sprintf(s, "%d-%d", iNumOfLine, iFiles);
    length = strlen(s) + 1;

    if (0 < msgsnd(currentThreadData->msqid, &sndmsg, length, IPC_NOWAIT)){
        perror("msgsnd");
        exit(1);
    }
   /////////////////////////////////////////////////////////////////////////////

    // thread'in islemi bittigi icin thread sonlandirilir.
    pthread_exit(NULL);
}


/**
 * Yapilan islemler main de kafa karistirmasin diye hepsini bu fonksiyonda 
 * topladim.
 *
 * sFileName: 
 * String, input, icinde arama yapilacak dosyanin adini tutuyor
 */
int searchStringInFile(char* sFileName, int pFd, pid_t pID, pid_t tID){
    char **sStr=NULL;
    int i=0, j=0;
    int iCount = 0;
    char s[MAXSIZE];
    //Burada adi verilen dosyanin acilip acilmadigina baktim
    //Acilamadiysa programi sonlandirdim.
    fPtrInFile = fopen (sFileName, "r");
    if (fPtrInFile == NULL) {
        perror (sFileName);
        fprintf(stderr, "  Exit Condition: due to error 1\n");
		sprintf(s, "  Exit Condition: due to error 1\n");
        write(logFifo, s, strlen(s));
        exit(1);
    }

    if(isEmpty(fPtrInFile) == 1){
        rewind(fPtrInFile);
        //Dosyanin satir sayisini ve en uzun satirin 
        //column sayisini bulan fonksiyonu cagirdim.
        findLengthLineAndNumOFline();
        
        //Dosyayi tekrar kapatip acmak yerine dosyanin nerede oldugunu 
        //gosteren pointeri dosyanin basina aldim
        rewind(fPtrInFile);

        //Dosyayi string arrayine okudum ve bu string'i return ettim
        sStr=readToFile();

        #ifndef DEBUG //Dosyayi dogru okuyup okumadigimin kontrolü
            printf("File>>>>>>>\n");
            for(i=0; i<iNumOfLine; i++)
                printf("%s\n", sStr[i]);
        #endif
        
        //s = (char*)calloc(MAXSIZE, sizeof(char));
        
        //String arrayi icinde stringi aradim ve sayisini iCount'a yazdim
        iCount=searchString(sFileName, sStr, pFd, pID, tID);
        //iWordCount += iCount;
        sprintf(s, "\n****************************************************\n");
        write(pFd, s, strlen(s));
        sprintf(s,"%s found %d in total in %s file\n", sSearchStr, iCount, sFileName); 
        write(pFd, s, strlen(s));
        sprintf(s, "****************************************************\n\n");
        write(pFd, s, strlen(s));
        
        //free(s);
        
        //Strin icin ayirdigim yeri bosalttim
        for(i=0; i<iNumOfLine; i++)
            free(sStr[i]);
        free(sStr);
    }
    fclose(fPtrInFile);
    
    fprintf(fPtrWords, "%d\n", iCount);
    
    return iCount;
}


/**
 * String arama isleminin ve her yeni bir string bulundugunda bulunan 
 * kelime sayisinin arttirildigi fonksiyon
 *
 * sFile     :String arrayi, input, ıcınde arama yapiacak string arrayi
 * sFileName :String'in aranacagi dosya adi
 * return degeri ise integer ve bulunan string sayisini return eder
 */
int searchString(char* sFileName, char **sFile, int pFd, pid_t pID, pid_t tID){
    int i=0, j=0;
    int iRow=0, iCol=0;
    char *word=NULL;
    int iCount=0;
    char s[MAXSIZE];    
    //string arrayinin her satirini sira ile str stringine kopyalayip inceleyecegim
    word=(char *)calloc(100, sizeof(char));
    //s = (char *)calloc(MAXSIZE, sizeof(char));
    for(i=0; i<iNumOfLine; i++){ //Satir sayisina gore donen dongu
        for(j=0; j<iLengthLine; j++){ //Sutun sayisina gore donen dongu
                //printf("i:%d\tj:%d\n", i, j);
            if((copyStr(sFile, word, i, j, &iRow, &iCol)) == 1){ //str stringine kopyalama yaptim
                //kopyalama ile sSearchStr esit mi diye baktim
                if(strncmp(word, sSearchStr, (int)strlen(sSearchStr)) == 0){
                    #ifndef DEBUG
                        printf("%d-%d %s: [%d, %d] %s first character is found.\n",pID, tID, sFileName, iRow, iCol, sSearchStr);
                    #endif
                	//Bulunan kelimenin satir ve sutun sayisi LogFile'a yazdim
                	sprintf(s, "%d-%d %s: [%d, %d] %s first character is found.\n", pID, tID, sFileName, iRow, iCol, sSearchStr);
                	write(pFd, s, strlen(s));
                    iCount++; //String sayisini bir arttirdim kelime buldugum icin
                }
            }
        }
    }
    //free(s);
    free(word);
    return iCount; //Bulunan string sayisini return ettim
}

/**
 * Aranmasi gereken stringin karakter sayisi kadar karakteri word stringine kopyalar.
 * Kopyalama yaparken kopyalanan karakterin space(' '), enter('\n') ve tab('\t') 
 * olmamasina dikkat ederek kopyalar.
 * 
 * sFile    :Dosyadaki karakterlerin tutuldugu iki boyutlu karakter arrayi
 * word     :Kopyalanan karakterlerin tutulacagi 1 karakter arrayi
 * iStartRow:Aramanin baslayacagi satir indexi
 * iStartCol:Aramanin baslayacagi sutun indexi
 * iRow     :Bulunan kelimenin ilk karakterinin bulundugu satir numarasi
 * iCol     :Bulunan kelimenin ilk karakterinin bulundugu sutun numarasi
 */
int copyStr(char **sFile, char* word, int iStartRow, int iStartCol, int *iRow, int *iCol){

    int k=0, i=0, j=0, jStart = 0;
    //printf("iStartRowIndex:%d\tiStartColIndex:%d\n", iStartRow, iStartCol);
    
    if(sFile[iStartRow][iStartCol] == '\n' || sFile[iStartRow][iStartCol] == '\t' || sFile[iStartRow][iStartCol] == ' '){
        return 0;
    }  
    else{
        *iRow = iStartRow+1;
        *iCol = iStartCol+1;
	    #ifndef DEBUG
    	    printf("iRow:%d\tiCol:%d\n", *iRow, *iCol);
		    printf("iStartRow:%d\tiStartCol:%d\n", iStartRow, iStartCol);
	    #endif
        k=0;
        jStart = *iCol-1;
        for(i=*iRow-1; i<iNumOfLine && k < iSizeOfSearchStr; i++){
            for(j=jStart; j<iLengthLine && k < iSizeOfSearchStr; j++){
		        if(sFile[i][j] != '\n' && sFile[i][j] != '\t' && sFile[i][j] != ' ' && k < iSizeOfSearchStr){
                    word[k] = sFile[i][j];
                    k++;
                }                
		        if(sFile[i][j] == '\n' && k < iSizeOfSearchStr){
                    j=iLengthLine;
                }
            }
	        jStart=0; //jnin bir alt satirda baslangic konumu 0 olarak ayarlandi
        }    
        if(k != iSizeOfSearchStr)
            return 0;
        else
            return 1;
    }
    return -1;
}



/**
 * dosyanin bos olup olmadigina bakar
 * 1->bos degil
 * 0->bos
 */
int isEmpty(FILE *file){
    long savedOffset = ftell(file);
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0){
        return 0;
    }

    fseek(file, savedOffset, SEEK_SET);
    return 1;
}


/**
 * Dosyadaki satir sayisini ve en uzun sutundaki karakter sayisini bulur.
 * Burdan gelen sonuclara gore dynamic allocation yapilir.
 */
void findLengthLineAndNumOFline(){
	int iLenghtLine=0;
	int iMaxSize=0;
	char ch=' ';

		while(!feof(fPtrInFile)){
			fscanf(fPtrInFile, "%c", &ch);
			iMaxSize++;
				if(ch == '\n'){
					iNumOfLine=iNumOfLine+1;
					if(iMaxSize >=(iLengthLine))
						iLengthLine=iMaxSize;
					iMaxSize=0;
				}
		}
		iNumOfLine-=1; //bir azalttim cunku dongu bir defa fazla donuyor ve iNumOfLine
                        //bir fazla bulunuyor.
        iLengthLine+=1;
        #ifndef DEBUG
            printf("iLengthLine:%d\tiNumOfLine:%d\n", iLengthLine, iNumOfLine);
        #endif
}

/**
 * Dosya okunur ve iki boyutlu bir karakter arrayyine atilir.
 * Karakter arrayi return edilir.    
 *
 * return : char**, okunan dosyayi iki boyutlu stringe aktardim ve 
 *          bu string arrayini return ettim.
 */
char** readToFile(){
    char **sFile=NULL;
    int i=0;
    char s[MAXSIZE];

    //Ikı boyutlu string(string array'i) icin yer ayirdim
    sFile=(char **)calloc(iNumOfLine*iLengthLine, sizeof(char*));
    if( sFile == NULL ){ //Yer yoksa hata verdim
        #ifndef DEBUG
            printf("INSUFFICIENT MEMORY!!!\n");
        #endif
        fprintf(stderr, "  Exit Condition: (INSUFFICIENT MEMORY) due to error 1\n");
		sprintf(s, "  Exit Condition: (INSUFFICIENT MEMORY) due to error 1\n");
        write(logFifo, s, strlen(s));
        exit(1);
    }
    //Ikı boyutlu oldugu ıcın her satir icinde yer ayirdim
    for(i=0; i<iNumOfLine; i++){
        sFile[i]=(char *)calloc(iLengthLine, sizeof(char));
        if( sFile[i] == NULL ){ //Yer yoksa hata verdim
            #ifndef DEBUG
                printf("INSUFFICIENT MEMORY!!!\n");
            #endif
            fprintf(stderr, "  Exit Condition: (INSUFFICIENT MEMORY) due to error 1\n");
	        sprintf(s, "  Exit Condition: (INSUFFICIENT MEMORY) due to error 1\n");
            write(logFifo, s, strlen(s));
            exit(1);
        }
    }

    i=0;
    do{ //Dosyayi okuyup string arrayine yazdim
    
        fgets(sFile[i], iLengthLine, fPtrInFile);
        #ifndef DEBUG
            printf("*-%s-*\n", sFile[i]);
        #endif
        i++;
    }while(!feof(fPtrInFile));

    return sFile;
}

/**
 * structure yapisindaki degerler initialize edildi.
 * temp dosyalari acildi
 * temp dosyalarinin pathleri tutuldu
 */
void openFiles(char path[MAXPATHSIZE]){
    numOfValues.iNumOfWords = 0;
    numOfValues.iNumOfDirectories = 0;
    numOfValues.iNumOfFiles = 0;
    numOfValues.iNumOfLines = 0;
    numOfValues.iNumOfProcess = 0;
    numOfValues.iNumOfThreads = 0;
    numOfValues.iTimeOperations = 0.0;
    
    
    fPtrWords = fopen("words.txt", "w+"); //file'lardaki string sayisinin yazildigi file
    fPtrDirs = fopen("dirs.txt", "w+");
    fPtrProcess = fopen("process.txt", "w+");
    fPtrFiles = fopen("files.txt", "w+");
    fPtrLines = fopen("lines.txt", "w+");
    fPtrThreads = fopen("threads.txt", "w+");
    
    
    sprintf(pathWords, "%s/words.txt", path);
    sprintf(pathDirs, "%s/dirs.txt", path);
    sprintf(pathProcess, "%s/process.txt", path);
    sprintf(pathFiles, "%s/files.txt", path);
    sprintf(pathLines, "%s/lines.txt", path);
    sprintf(pathThreads, "%s/threads.txt", path);
}

/**
 * acilan temp dosyalar kapatildi.
 * temp dosyalar kaldirildi
 */ 
void closeFiles(){
    fclose(fPtrWords);
    fclose(fPtrDirs);
    fclose(fPtrProcess);
    fclose(fPtrFiles);
    fclose(fPtrLines);
    fclose(fPtrThreads);
    
    remove(pathWords);
    remove(pathDirs);
    remove(pathProcess);
    remove(pathFiles);
    remove(pathLines);
    remove(pathThreads);
    
    remove(allpath);
}

/**
 * gereken degerler hesaplandi.
 */
void calculateNumOfValues(){
    numOfValues.iNumOfWords = 0;
    numOfValues.iNumOfDirectories = 0;
    numOfValues.iNumOfFiles = 0;
    numOfValues.iNumOfLines = 0;
    numOfValues.iNumOfProcess = 0;
    numOfValues.iNumOfThreads = 0;
    
    //string sayisi hesaplandi
    int i = 0;
    rewind(fPtrWords);
    while(!feof(fPtrWords)){
        fscanf(fPtrWords, "%d", &i);
        numOfValues.iNumOfWords += i;
    }
    numOfValues.iNumOfWords -= i; //1 tane fazladan okuyor
    
    //directory sayisi hesaplandi
    i=0;
    rewind(fPtrDirs);
    while(!feof(fPtrDirs)){
        fscanf(fPtrDirs, "%d", &i);
        numOfValues.iNumOfDirectories += i;
    }
    numOfValues.iNumOfDirectories -= i; //1 tane fazladan okuyor
    
    //file sayisi hesaplandi
    i = 0;
    rewind(fPtrFiles);
    while(!feof(fPtrFiles)){
        fscanf(fPtrFiles, "%d", &i);
        numOfValues.iNumOfFiles += i;
    }
    numOfValues.iNumOfFiles -= i; //1 tane fazladan okuyor
    
    //okunan satir sayisi hesaplandi
    i = 0;
    rewind(fPtrLines);
    while(!feof(fPtrLines)){
        fscanf(fPtrLines, "%d", &i);
        numOfValues.iNumOfLines += i;
    }
    numOfValues.iNumOfLines -= i; //1 tane fazladan okuyor

    //olusturulan process sayisi hesaplandi
    i = 0;
    rewind(fPtrProcess);
    while(!feof(fPtrProcess)){
        fscanf(fPtrProcess, "%d", &i);
        numOfValues.iNumOfProcess  += i;
    }
    numOfValues.iNumOfProcess  -= i; //1 tane fazladan okuyor
    numOfValues.iNumOfDirectories = numOfValues.iNumOfProcess;
    
    numOfValues.iNumOfCascadeThreads = numOfValues.iNumOfDirectories;
    //olusturulan toplam thread sayisi hesaplandi
    i = 0;
    rewind(fPtrThreads);
    while(!feof(fPtrThreads)){
        fscanf(fPtrThreads, "%d", &i);
        numOfValues.iNumOfThreads += i;
    }
    numOfValues.iNumOfThreads -= i; //1 tane fazladan okuyor   
    
    numOfValues.iNumOfMaxThreads = iMaxThreads; 
}

/**
 * Gelen sinyal yakalanir 
 * Gerekli cikis islemi gerceklestirilir
 * Hangi sinyal nedeni ile cikildigi yazilir
 */
void signalHandle(int sig){
    char s[MAXSIZE];
    printf("  Exit Condition: due to signal no:%d\n", sig);
    printf("*******************************************************\n\n");
    write(logFifo, s, strlen(s)); 
    sprintf(s, "  Exit Condition: due to signal no:%d\n", sig);
    write(logFifo, s, strlen(s)); 
    sprintf(s, "*******************************************************\n\n");
    write(logFifo, s, strlen(s));
    closeFiles();
    exit(1);
}

///////////////////////////////////////////////////////////////////////////////
