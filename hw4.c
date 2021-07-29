#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>

typedef struct Device{
	char name[20];
	int  obsNum;
	int avg;
	sem_t s_device;
}*pDevice,Device;

typedef struct MyObs{
  	char obsName[20];
	int obsNum;
	long start;
	long finish;
	bool iDidIt ;
	
}*pMyObs,MyObs;

typedef struct Member{
	char name[20];
	int strangth;
	long startTime;
	long stopTime;
	long waitTime;
	pMyObs myobs;
	sem_t noObs; //Semaphore for enter to wating list
}*pMember,Member;

/*Global variables*/
int K; 	//number of devices types
int M;	//number of team members
int D; 	// Total devices number

sem_t waitingList;
pDevice devicesArr;
pMember membersArr;
pMember *m_WatingList;
int last=0; //last index availble in waitingList

/*Function declarations*/
void initDevices();
void initMembers();
void *workout(void* index);
void printTimes();
void doDevice(char deviceName[],int member,int fIndex);
bool checkIfAlldeviceAreDone(int member);
int foundFirstIndex(char deviceName[]);
void enterToWatingList(int member);
void wakeUp();

int main(){
	int i;
	pthread_t * pthreadArray;
	int *index;
	initDevices();
	initMembers();
	sem_init(&waitingList,0,1); //init of WatingList semaphore

	 /*Memory allocation*/
    pthreadArray = malloc (sizeof(pthread_t) * M);
    index = malloc (sizeof(int) * M);
    if (index == NULL||pthreadArray==NULL){
        printf("Allocation failure!");
        exit(1);
    }
	m_WatingList=malloc(M*sizeof(Member*)); //arr of pointers to members
		if(m_WatingList==NULL){
			perror("allocation for WatingList faild!\n");
		exit(1);
		}
	
	/*Creating threads*/
	for(i=0;i<M;i++){;
		index[i]=i;
     if(pthread_create(&pthreadArray[i],NULL,workout,&index[i]) !=0){
        printf("\n faild to create threads");//if could not create thread faild
        exit(-1);
	 }
	}

	/*Forcing main to wait for all threads to finish*/
	for(i=0;i<M;i++)
		pthread_join(pthreadArray[i],0);
	
	/*prints members data*/
	printTimes();	

	/*Relasing alocated memory*/
	free(m_WatingList);
	free(devicesArr);
	for(i=0; i<M;i++){
		free(membersArr[i].myobs);
	}
	free(membersArr);
	return 0;
}

void initDevices(){
	int course_desc,rbytes,numOfRows,numOfTotalObs=0,numOfSame=0,avg,temp,i,j=0,k;
	char buffer[1],str[128]="",str2[128]="",str3[128]="";

	course_desc = open("course_desc.txt",O_RDONLY);
    if(course_desc == -1){
        perror("could not open course_desc.txt\n");
        exit(1);
    }

	while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\n')
		strncat(str,buffer, 1);
	numOfRows=atoi(str);
	strcpy(str,"");

	for(i = 0;i < numOfRows;i++){
		/*we skip name of obs*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\t');

		/*we get num of obs*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\t')
			
			strncat(str,buffer, 1);
		numOfTotalObs+=atoi(str);	
		strcpy(str,"");
		/*we skip avg time of obs*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\n');
	}
	D=numOfTotalObs;
	//close(course_desc);
	/*allocation for devicesArr*/
	devicesArr=malloc(D*sizeof(Device));
	if(devicesArr==NULL){
		perror("allocation for devicesArr faild!\n");
		exit(1);
	}
	lseek( course_desc, 0, SEEK_SET ); 
	/*skip number of rows*/
	while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\n');
	for(i = 0;i < numOfRows;i++){
		/*get name of device*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\t')
			
			strncat(str,buffer, 1);
		/*we get num device*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\t')
			
			strncat(str2,buffer, 1);
		numOfSame+=atoi(str2);
		/*we get avg for device*/
		while((rbytes = read (course_desc, &buffer, 1)) > 0 && buffer[0]!='\n')
			strncat(str3,buffer, 1);
		avg=atoi(str3);
		/*for same device*/
		k=0;
		for(j=j; j <numOfSame; j++){
			strcat(devicesArr[j].name,str);
			devicesArr[j].obsNum=k;
			devicesArr[j].avg=avg;
			sem_init(&devicesArr[j].s_device,0,1);
			k++;
		}
		
		/*reset strings*/
		strcpy(str,"");
		strcpy(str2,"");
		strcpy(str3,"");
	}
	K = numOfRows;
}


void initMembers(){
	char buffer[1],str[128]="";
	int rbytes,team_desc,i,j=0,t=0,numOfRows;

	team_desc = open("team_desc.txt",O_RDONLY);
    if(team_desc == -1){
        perror("could not open team_desc.txt\n");
        exit(1);
    }

	while((rbytes = read (team_desc, &buffer, 1)) > 0 && buffer[0]!='\n')
		strncat(str,buffer, 1);
	numOfRows=atoi(str);
	M = numOfRows;
		/*allocation for membersArr*/
	membersArr=malloc(M*sizeof(Member));
	if(membersArr==NULL){
		perror("allocation for membersArr faild!\n");
		exit(1);
	}
	for(i = 0;i < M;i++){
		strcpy(str,"");
		/*we get name of members*/
		while((rbytes = read (team_desc, &buffer, 1)) > 0 && buffer[0]!='\t')
			strncat(str,buffer,1);
			strcpy(membersArr[i].name,str);
			strcpy(str,"");
		/*we get num of strangth*/
		while((rbytes = read (team_desc, &buffer, 1)) > 0 && buffer[0]!='\n')
			strncat(str,buffer, 1);
		membersArr[i].strangth=atoi(str);	
		strcpy(str,"");
		membersArr[i].myobs=malloc(K*sizeof(MyObs)); //Shlomi's
		if(membersArr[i].myobs==NULL){
			perror("allocation for membersArr[i].myobs faild!\n");
		exit(1);
		}	
			j=0;
			strcpy(membersArr[i].myobs[0].obsName,devicesArr[j++].name);
			for(t=1;t<K;t++){
				while(strcmp(membersArr[i].myobs[t-1].obsName,devicesArr[j].name)==0)
					j++;
				strcpy(membersArr[i].myobs[t].obsName,devicesArr[j].name);
				membersArr[i].myobs[t].iDidIt=false;
				sem_init(&membersArr[i].noObs,0,0);
				membersArr[i].waitTime = 0;
			}
			
	}
	close(team_desc);
}

void printTimes(){ //printing all the data
	int i=0,j;
	long active=0,elapsed;
	for(i=0;i<M;i++){
		active=0;
		printf("%s results:\n",membersArr[i].name);
		printf("\tenter: %ld\n",membersArr[i].startTime);
		for(j=0;j<K;j++){
			elapsed=membersArr[i].stopTime-membersArr[i].startTime;
			active+=membersArr[i].myobs[j].finish-membersArr[i].myobs[j].start;
			printf("\t%d.\t%s\tobs: %d start:\t%ld\tfinish:\t%ld\tduration: %ld\n",j,membersArr[i].myobs[j].obsName,
			 membersArr[i].myobs[j].obsNum,membersArr[i].myobs[j].start,membersArr[i].myobs[j].finish,membersArr[i].myobs[j].finish-membersArr[i].myobs[j].start);
		}
		printf("\texit: %ld\telapsed: %ld\tactive: %ld\twait: %ld\n\n",membersArr[i].stopTime,elapsed,active,membersArr[i].waitTime);
	}
}

void *workout(void* index){
	int i,m_index=*(int*)index,counter=0,fIndex;
	char name[20]="";
	bool compleated = false;
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	membersArr[m_index].startTime= current_time.tv_sec*1000 + current_time.tv_usec/1000; //Updating start time of member;
	while(!compleated){
		for(i=0;i<K;i++){
			if(!membersArr[m_index].myobs[i].iDidIt){                         //If the member didn't do the device yet.
				strcpy(name,membersArr[m_index].myobs[i].obsName);
				fIndex=foundFirstIndex(name);
				while(strcmp(devicesArr[fIndex].name,name)==0&&fIndex<D){   //while is the same name and in the bounderies
					if(sem_trywait(&devicesArr[fIndex].s_device)==-1)	//not availble device
						fIndex++;
					else{
						doDevice(name,m_index,fIndex);
						i=0;
						break;
					}
				}
			}
		}
		if(checkIfAlldeviceAreDone(m_index)){ //member done all devices?
			compleated=true;
			gettimeofday(&current_time, NULL);
			membersArr[m_index].stopTime= current_time.tv_sec*1000 + current_time.tv_usec/1000; //Updating stop time of member;
		}
		else{ //wating list
			enterToWatingList(m_index);
		}
	}
}

bool checkIfAlldeviceAreDone(int member){
	int i;
	for(i=0;i<K;i++)
		if(!membersArr[member].myobs[i].iDidIt)
			return false;
	return true;
}

int foundFirstIndex(char deviceName[]){ //finds first index of device in device arr
	int i=0;
	for(i=0;i<D;i++){
		if(strcmp(deviceName,devicesArr[i].name)==0)
			return i;
	}
	return -1;
}
void doDevice(char deviceName[],int member,int fIndex){ //member working on decive 
	int i=0,avgTime,rand1;
	struct timeval current_time;
	//reduce avg time by member's strangth
	for(i=0;i<D;i++){
		if(strcmp(deviceName,devicesArr[i].name)==0){
			avgTime=devicesArr[i].avg; //recived avg time
			i=D;//breaking for loop
		}
	}
	avgTime+=(avgTime/100)*membersArr[member].strangth; //we assume that -strangth = better time 
	rand1 =(rand() % (21)) -10;
	//reduce new time up to 10% randomly
	avgTime+=(avgTime/100)*rand1;
	//start time of device 
	for(i=0;i<K;i++){
		if(strcmp(membersArr[member].myobs[i].obsName,deviceName)==0){
			gettimeofday(&current_time, NULL);
			membersArr[member].myobs[i].start=current_time.tv_sec*1000 + current_time.tv_usec/1000;
			membersArr[member].myobs[i].obsNum = devicesArr[fIndex].obsNum;
			break;//break the loop
		}	
	}
	//sleep time(do device)
	usleep(avgTime*1000);
	//end time
	gettimeofday(&current_time, NULL);
	membersArr[member].myobs[i].finish=current_time.tv_sec*1000 + current_time.tv_usec/1000;
	//flag that we did the device in the personal device list
	membersArr[member].myobs[i].iDidIt=true;
	//post the semphore
	sem_post(&devicesArr[fIndex].s_device);
	//Wake all the waiting list
	wakeUp();
}

void enterToWatingList(int member){
	struct timeval current_time;
	long start,finish;
	int i=0;
	//locks wating list
	sem_wait(&waitingList);
	//insert to waiting list in the next avialble space
	m_WatingList[last++]=&membersArr[member];
	//free lock waiting list
	sem_post(&waitingList);
	//start count waiting time
	gettimeofday(&current_time, NULL);
	start=current_time.tv_sec*1000 + current_time.tv_usec/1000;
	//wait on the member's sem
	sem_wait(&membersArr[member].noObs);
	//exit time for wating
	gettimeofday(&current_time, NULL);
	finish=current_time.tv_sec*1000 + current_time.tv_usec/1000;
	//add to member his waiting time
	membersArr[member].waitTime += finish-start;
}

void wakeUp(){
	int i = 0;
	sem_wait(&waitingList);
	for(i=0;i<last;i++){
		if(m_WatingList[i]!=NULL)//if this spot is not empty
		sem_post(&m_WatingList[i]->noObs);
		m_WatingList[i]=NULL;
	}
	last=0;
	sem_post(&waitingList);
}

