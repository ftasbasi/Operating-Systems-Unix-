#include "writeOutput.h"
#include "furkan.h"
#include <iostream>
#include <string.h>
#include <sstream>
#include <semaphore.h>
#include <stdlib.h> 
#include <unistd.h> 
//GLOBALDefinitions
using namespace std;
sem_t mutex;
sem_t* mutexesMiner;
sem_t* mutexesSmelter;
sem_t* mutexesFoundry;
sem_t MinerProducedSema;
sem_t* canProduce;
sem_t activeMiner_Controller;
sem_t* activeSmelter_Controller;
sem_t SmelterProduced;
sem_t* unloaded_smelter;

int MinerProduced;
typeMiner *miners;
typeSmelter *smelters;

int Nmines;
int Nsmelters;
int activeMiner;	//pointer array for checking a Miner active or not
int* activeSmelters;
sem_t* smelterProduced_Controller;
//endGLOBALDefinitions

void *threadMINERR(void * arg){
    //mining
    unsigned int counter=0;
    unsigned int time;
    //wait
    unsigned int mutexIndex;

    typeMiner *miner = (typeMiner *) (arg);
    time=miner->Im;    
    mutexIndex=miner->ID-1;
    miner->current_count=0;

    MinerInfo* tmpminer;
    tmpminer =(MinerInfo *)malloc(sizeof(MinerInfo));

    sem_wait(&activeMiner_Controller);
    activeMiner+=1;                //broadcasting "I'M ACTIVE"
    sem_post(&activeMiner_Controller);    
    
    while (1)
    {
    	sem_wait(&canProduce[mutexIndex]); //can I produce?

		sem_wait(&mutexesMiner[mutexIndex]);
        FillMinerInfo(tmpminer,miner->ID,miner->oreType,miner->capacity,miner->current_count);
        WriteOutput(tmpminer, NULL, NULL, NULL, MINER_STARTED);
        miner->current_count+=1;
        usleep(time - (time*0.01) + (rand()%(int)(time*0.02))); 
        counter+=1;
        FillMinerInfo(tmpminer,miner->ID,miner->oreType,miner->capacity,miner->current_count);
        WriteOutput(tmpminer, NULL, NULL, NULL, MINER_FINISHED);
        
        sem_wait(&MinerProducedSema);
        MinerProduced+=1;

        sem_post(&MinerProducedSema);
        
        if(counter==miner->Rm){
            FillMinerInfo(tmpminer,miner->ID,miner->oreType,miner->capacity,miner->current_count);
            WriteOutput(tmpminer, NULL, NULL, NULL, MINER_STOPPED);
            activeMiner-=1; //broadcasting "I'M PASSIVE"
            sem_post(&mutexesMiner[mutexIndex]);
            break;
        }else{
        	sem_post(&mutexesMiner[mutexIndex]);
        }
    }
    free(tmpminer);
}

void *threadTRANSPORTERR(void *arg)
{

	unsigned int time;
    typeTransporter *transporter = (typeTransporter *) (arg);
    time=transporter->It;    
    transporter->carry=NULL;
    int work=1;
    int indexofSearch=0;

    MinerInfo* tmpminer;
    tmpminer =(MinerInfo *)malloc(sizeof(MinerInfo));
    TransporterInfo* tmptransporter;
    tmptransporter =(TransporterInfo *)malloc(sizeof(TransporterInfo));
    SmelterInfo* tmpsmelter;
    tmpsmelter =(SmelterInfo *)malloc(sizeof(SmelterInfo));


	while(work)
	{
 	
    	work=0;
    	OreType carryType;

		sem_wait(&activeMiner_Controller);	

		if (activeMiner>0)
		{
			work=1;
		}
		sem_post(&activeMiner_Controller);

		sem_wait(&MinerProducedSema);
		sem_wait(&SmelterProduced);

		if (MinerProduced>0)
		{	
			work=1;
		}
		sem_post(&MinerProducedSema);

		if (work==0)
		{	
			FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry);
			WriteOutput( NULL, tmptransporter,NULL, NULL, TRANSPORTER_STOPPED);
			break;
		}

		transporter->carry=NULL;
		while(1){
			int counter=0;
			

			sem_wait(&mutexesMiner[indexofSearch]);
			//printf("\n mutexesMiner[%d]  first \n",indexofSearch);
			

			if(miners[indexofSearch].current_count>0){
				sem_post(&mutexesMiner[indexofSearch]);
				//printf("\nIM HERE1\n");
				break;
			}else{
				//printf("\n IM HERE \n");
				sem_post(&mutexesMiner[indexofSearch]);

			}

			indexofSearch+=1;
			if (indexofSearch==Nmines)
			{
				indexofSearch=0;
			}
		
		}
		//do staff
		
		sem_wait(&mutexesMiner[indexofSearch]);
		FillMinerInfo(tmpminer,miners[indexofSearch].ID,(OreType) 0,0,0);
		FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry);
		WriteOutput(tmpminer, tmptransporter,NULL, NULL, TRANSPORTER_TRAVEL); //TRAVEL to MINER to get ore
		
		usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

		miners[indexofSearch].current_count-=1;
		carryType=miners[indexofSearch].oreType;
		transporter->carry=&miners[indexofSearch].oreType;
		FillMinerInfo(tmpminer,miners[indexofSearch].ID,miners[indexofSearch].oreType,miners[indexofSearch].capacity,miners[indexofSearch].current_count);
		FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry);
		WriteOutput(tmpminer, tmptransporter,NULL, NULL, TRANSPORTER_TAKE_ORE); //TRAVEL FOR TAKING ORE FROM MINER
		
		sem_wait(&MinerProducedSema);

		//printf("\n MinerProducedSema : 2\n");

		if (MinerProduced>0)
		{
			MinerProduced-=1;
		//printf("\n MinerProduced :%d \n", MinerProduced);
		}
		sem_post(&MinerProducedSema);

		
		sem_post(&mutexesMiner[indexofSearch]);
		sem_post(&canProduce[indexofSearch]);
		
		usleep(time - (time*0.01) + (rand()%(int)(time*0.02))); 
		//TRANSPORTER_ MINER_ROUTINE_ENDS_HERE
		

		if (carryType!=2)
		{
		
		int checkPriority=0;
		int priorityIndex=0;

		int checkEmpty=0;
		int emptyIndex=0;




		for (int jj = 0; jj < Nsmelters; jj++)
		{	
		//printf("\n SMELTER %d : waiting_ore_count-> :%d \n",smelters[jj].ID,smelters[jj].waiting_ore_count);
			sem_wait(&mutexesSmelter[jj]);

			if (smelters[jj].waiting_ore_count>0 && (smelters[jj].waiting_ore_count % 2 ==1) && carryType==smelters[jj].oreType)
			{
				checkPriority=1;
				priorityIndex=jj;
				sem_post(&mutexesSmelter[jj]);
				break;
			}else{
				sem_post(&mutexesSmelter[jj]);
			}
			
		}

		if (checkPriority==0)
		{	//case1
			//priority not exist
			for (int jj = 0; jj < Nsmelters;jj++){
				sem_wait(&mutexesSmelter[jj]);
				if (smelters[jj].waiting_ore_count==0 && carryType==smelters[jj].oreType)
				{
					checkEmpty=1;
					emptyIndex=jj;
					sem_post(&mutexesSmelter[jj]);
					break;
				}else{
					sem_post(&mutexesSmelter[jj]);
				}
				
			}
		}
		
		if(checkPriority){
			sem_wait(&mutexesSmelter[priorityIndex]);
			FillSmelterInfo(tmpsmelter,smelters[priorityIndex].ID,(OreType)0,0,0,0);
			FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry); //may be we need to lock it with transporter semaphore
			WriteOutput(NULL,tmptransporter,tmpsmelter,NULL,TRANSPORTER_TRAVEL);
			usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));
			smelters[priorityIndex].waiting_ore_count+=1;


			FillSmelterInfo(tmpsmelter,smelters[priorityIndex].ID,smelters[priorityIndex].oreType,smelters[priorityIndex].loading_capacity,smelters[priorityIndex].waiting_ore_count,smelters[priorityIndex].total_produce);
			FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry); //may be we need to lock it with transporter semaphore
			WriteOutput(NULL,tmptransporter,tmpsmelter,NULL,TRANSPORTER_DROP_ORE);
			usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

			if (smelters[priorityIndex].waiting_ore_count>1)
			{
				sem_post(&unloaded_smelter[priorityIndex]); //Unloaded smelter /wait for 2
			}
			
			sem_post(&mutexesSmelter[priorityIndex]);
		}else if(checkEmpty){

			sem_wait(&mutexesSmelter[emptyIndex]);
			

			FillSmelterInfo(tmpsmelter,smelters[emptyIndex].ID,(OreType)0,0,0,0);
			FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry); //may be we need to lock it with transporter semaphore
			WriteOutput(NULL,tmptransporter,tmpsmelter,NULL,TRANSPORTER_TRAVEL);
			usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));
			smelters[emptyIndex].waiting_ore_count+=1;
			
			FillSmelterInfo(tmpsmelter,smelters[emptyIndex].ID,smelters[emptyIndex].oreType,smelters[emptyIndex].loading_capacity,smelters[emptyIndex].waiting_ore_count,smelters[emptyIndex].total_produce);
			FillTransporterInfo(tmptransporter, transporter->ID, transporter->carry); //may be we need to lock it with transporter semaphore
			WriteOutput(NULL,tmptransporter,tmpsmelter,NULL,TRANSPORTER_DROP_ORE);
			usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));

			if (smelters[emptyIndex].waiting_ore_count>1)
			{
				sem_post(&unloaded_smelter[emptyIndex]); //Unloaded smelter /wait for 2
			}

			
			sem_post(&mutexesSmelter[emptyIndex]);


		}else{
			//no such case
			
		}

		}
		

		

		indexofSearch+=1;
		if (indexofSearch==Nmines)
		{
			indexofSearch=0;
		}


	}
	
	free(tmpminer);
   	free(tmptransporter);
    free(tmpsmelter);

}

void *threadSMELTERR(void * arg)
{

    //miningINGOT
    unsigned int ProducedIngotCount=0;
    unsigned int time;
    unsigned int smelterIndex;
    typeSmelter *smelter = (typeSmelter *) (arg);
    time=smelter->Is;     //interval of smelter
    smelterIndex=smelter->ID-1;

    SmelterInfo* tmpsmelter;
    tmpsmelter =(SmelterInfo *)malloc(sizeof(SmelterInfo));
    sem_wait(&mutexesSmelter[smelterIndex]);
    FillSmelterInfo(tmpsmelter,smelter->ID,smelter->oreType,smelter->loading_capacity,smelter->waiting_ore_count,smelter->total_produce);
    WriteOutput(NULL,NULL,tmpsmelter,NULL,SMELTER_CREATED);
	sem_post(&mutexesSmelter[smelterIndex]);
   

	for (int i = 0; i < smelter->loading_capacity; i++)
	{
		sem_post(&SmelterProduced);
	}
    /////////////////////////////////////////////////////timeout tasks
    struct timespec tm;
    
    int work=1;

	/////////////////////////////////////////////////////timeout tasks
	sem_wait(&activeSmelter_Controller[smelterIndex]);
    activeSmelters[smelterIndex]=1;                //broadcasting "I'M ACTIVE"
    sem_post(&activeSmelter_Controller[smelterIndex]);    
   

   
    while (work)
    {	
    	int i=0;
    	do {
	        clock_gettime(CLOCK_REALTIME, &tm);
	        tm.tv_sec += 1;
	        i++;
	        if (i>=5){
	        	work=0;
	        	break;
	        }
	        
    	} while ( sem_timedwait(&unloaded_smelter[smelterIndex], &tm ) == -1 );

    	if (work==0)
    	{
    		break;
    	}

		sem_wait(&mutexesSmelter[smelterIndex]);

		FillSmelterInfo(tmpsmelter,smelters[smelterIndex].ID,smelters[smelterIndex].oreType,smelter->loading_capacity,smelters[smelterIndex].waiting_ore_count,ProducedIngotCount);
    	WriteOutput(NULL,NULL,tmpsmelter,NULL,SMELTER_STARTED);
    	usleep(time - (time*0.01) + (rand()%(int)(time*0.02)));
    	if (smelters[smelterIndex].waiting_ore_count>=2)
    	{


    		smelters[smelterIndex].waiting_ore_count=smelters[smelterIndex].waiting_ore_count-2;
    		smelters[smelterIndex].total_produce+=1;
    		ProducedIngotCount+=1;
    		sem_post(&SmelterProduced);
    		sem_post(&SmelterProduced);


    	}
    	
		FillSmelterInfo(tmpsmelter,smelters[smelterIndex].ID,smelters[smelterIndex].oreType,smelters[smelterIndex].loading_capacity,smelters[smelterIndex].waiting_ore_count,smelters[smelterIndex].total_produce);
    	WriteOutput(NULL,NULL,tmpsmelter,NULL,SMELTER_FINISHED);

		sem_post(&mutexesSmelter[smelterIndex]);


    }

	sem_wait(&activeSmelter_Controller[smelterIndex]);
	
	activeSmelters[smelterIndex]=0;                //broadcasting SMELTERSTOPPED() psuedo code is here

	sem_post(&activeSmelter_Controller[smelterIndex]);   	

	sem_wait(&mutexesSmelter[smelterIndex]);
    FillSmelterInfo(tmpsmelter,smelters[smelterIndex].ID,smelters[smelterIndex].oreType,smelters[smelterIndex].loading_capacity,smelters[smelterIndex].waiting_ore_count,smelters[smelterIndex].total_produce);
    WriteOutput(NULL,NULL,tmpsmelter,NULL,SMELTER_STOPPED);
	sem_post(&mutexesSmelter[smelterIndex]);
	free(tmpsmelter);
}

int main()
{
	//total_producedd=0;
	//Take MINERS
	char inp[100]="";
	fgets(inp,100,stdin);
	Nmines=atoi(inp);
	const char s[2] = " "; //USED FOR INPUT PARSING
	/////////////////////////////////////////////////////////////MINER ALLOCATIONS
	miners =(typeMiner *)malloc(Nmines * sizeof(typeMiner));
	char minerComponents[4][100];
	

	////////////////////////////////////////////////////SEMAPHORES

	mutexesMiner=(sem_t *)malloc(Nmines*sizeof(sem_t));
	canProduce=(sem_t *)malloc(Nmines*sizeof(sem_t));
	

	////////////////////////////////////////////////////SEMAPHORES

	///////////////////TEMP ALLOCATIONS//////////////////////
	MinerInfo* tmpminer;
    tmpminer =(MinerInfo *)malloc(sizeof(MinerInfo));
    TransporterInfo* tmptransporter;
    tmptransporter =(TransporterInfo *)malloc(sizeof(TransporterInfo));
    SmelterInfo* tmpsmelter;
    tmpsmelter =(SmelterInfo *)malloc(sizeof(SmelterInfo));
    FoundryInfo* tmpfoundry;
    tmpfoundry =(FoundryInfo *)malloc(sizeof(FoundryInfo));
    /////////////////////////////////////////////////////////
#pragma endregion GENERAL STAFF

#pragma region "MINER_INPUT_STAFF"

	sem_init(&MinerProducedSema,0,1);
	for (int i = 0; i < Nmines; i++)
	{	
		miners[i].ID=i+1;
		fgets(inp,100,stdin);

		char *token;

		/* get the first token */
		token = strtok(inp, s);

		/* walk through other tokens */
		int index=0;
		while( token != NULL ) {
	
		if(index==0){
			miners[i].Im=atoi(token);
		}else if(index==1){
			miners[i].capacity=atoi(token);
		}else if (index==2){
			miners[i].oreType=setOre(atoi(token));
		}else if (index==3){
			miners[i].Rm=atoi(token);
		}else{
			break;
		}

		token = strtok(NULL, s);
		index++;

	}
		miners[i].current_count=0;
	}
#pragma endregion "MINER_INPUT_STAFF"



	//Take TRANSPORTERS
	fgets(inp,100,stdin);	
	int Ntransporters;
	Ntransporters=atoi(inp);
	
	typeTransporter *transporters;
	transporters =(typeTransporter *)malloc(Ntransporters * sizeof(typeTransporter));	
	char transporterIt[100];

#pragma region "TRANSPORTER_INPUT_STAFF"
	for (int i = 0; i < Ntransporters; i++)
	{	
		transporters[i].ID=i+1;
		fgets(inp,100,stdin);

			transporters[i].It=atoi(inp);
			transporters[i].carry=NULL;//en basta ne tasidigi bilgisi NULL
	}
#pragma endregion "TRANSPORTER_INPUT_STAFF"	

#pragma region "SMELTER_INPUT_STAFF"
	//Take SMELTERS

	fgets(inp,100,stdin);	

	Nsmelters=atoi(inp);
	////ALLOCATIONS OF SMELTER
	smelters =(typeSmelter *)malloc(Nsmelters * sizeof(typeSmelter));

	mutexesSmelter=(sem_t *)malloc(Nsmelters*sizeof(sem_t));
	activeSmelters=(int*)malloc(sizeof(int));
	activeSmelter_Controller=(sem_t *)malloc(Nsmelters*sizeof(sem_t));

	smelterProduced_Controller=(sem_t *)malloc(Nsmelters*sizeof(sem_t));
	unloaded_smelter=(sem_t *)malloc(Nsmelters*sizeof(sem_t));
	typeSmelter smelters[Nsmelters];

	

	////ALLOCATIONS OF SMELTER



	char smeltersComponents[3][100];
	for (int i = 0; i < Nsmelters; i++)
	{	
		smelters[i].ID=i+1;
		fgets(inp,100,stdin);

		char *token;

		/* get the first token */
		token = strtok(inp, s);

		/* walk through other tokens */
		int index=0;
		while( token != NULL ) {

		
		if(index==0){
			smelters[i].Is=atoi(token);
		}else if(index==1){
			smelters[i].loading_capacity=atoi(token);
		}else if (index==2){
			smelters[i].oreType=setOre(atoi(token));
		}else{
			break;
		}

		token = strtok(NULL, s);
		index++;
	}
	sem_init(&smelterProduced_Controller[i],0,1);
	
	smelters[i].total_produce=0;
	smelters[i].waiting_ore_count=0;	
	}
	sem_init(&SmelterProduced,0,0);
#pragma endregion "SMELTER_INPUT_STAFF"
#pragma region "FOUNDRY_INPUT_STAFF"
	//Take Foundries

	fgets(inp,100,stdin);	
	int NFoundries;
	NFoundries=atoi(inp);

	typeFoundry foundries[NFoundries];
	char foundriesComponents[3][100];
	for (int i = 0; i < NFoundries; i++)
	{	
		foundries[i].ID=i+1;
		fgets(inp,100,stdin);

		char *token;

		/* get the first token */
		token = strtok(inp, s);

		/* walk through other tokens */
		int index=0;
		while( token != NULL ) {

		
		if(index==0){
			foundries[i].Finterval=atoi(token);
		}else if(index==1){
			foundries[i].loading_capacity=atoi(token);
		}else{
			break;
		}

		token = strtok(NULL, s);
		index++;
	}

	foundries[i].total_produce=0;
	foundries[i].waiting_iron=0;
	foundries[i].waiting_coal=0;


	}
#pragma endregion "FOUNDRY_INPUT_STAFF"

#pragma region TODO BEFORE CREATION OF THREADS
	InitWriteOutput();
	//CREATION OF THREADS
	pthread_t minerThreads[Nmines]; 
    pthread_t transporterThreads[Ntransporters];
    pthread_t smelterThreads[Nsmelters];
    pthread_t foundryThreads[NFoundries];
    for (int i = 0; i < Nmines; i++)
    {
    	sem_init(&mutexesMiner[i], 0, 1);
    	sem_init(&activeMiner_Controller, 0, 1);
    	sem_init(&canProduce[i], 0, miners[i].capacity); // a semaphore that indicates a miner can produce an ore which is signaled by tranportes in TAKE_ORE step
    	pthread_create(&minerThreads[i], NULL, threadMINERR,&miners[i]);
    	FillMinerInfo(tmpminer,miners[i].ID,miners[i].oreType,miners[i].capacity,miners[i].current_count);
    	WriteOutput(tmpminer, NULL, NULL, NULL, MINER_CREATED);
    }
    for (int i = 0; i < Ntransporters; i++)
    {
    	pthread_create(&transporterThreads[i], NULL, threadTRANSPORTERR, &transporters[i]);
    	FillTransporterInfo(tmptransporter,transporters[i].ID,transporters[i].carry);
    	WriteOutput( NULL, tmptransporter,NULL, NULL, TRANSPORTER_CREATED);
    }
   
    for (int i = 0; i < Nsmelters; i++)
    {	
    	activeSmelters[i]=0;
    	sem_init(&mutexesSmelter[i], 0, 1);
    	smelters[i].waiting_ore_count=0;
    	sem_init( &unloaded_smelter[i], 0, 0);
    	sem_init(&activeSmelter_Controller[i], 0, 1);
    	pthread_create(&smelterThreads[i], NULL, threadSMELTERR, &smelters[i]);
    }
    for (int i = 0; i < NFoundries; i++)
    {

    	pthread_create(&foundryThreads[i], NULL, threadFOUNDRYY, &foundries[i]);
    	WriteOutput( NULL, NULL, NULL,tmpfoundry ,FOUNDRY_CREATED);
    }

#pragma endregion TODO BEFORE CREATION OF THREADS
#pragma region ENDSTAFF
    //EXITING
    for (int i = 0; i < Nmines; i++)
    {
 		pthread_join(minerThreads[i], NULL);
 		
	}
	for (int i = 0; i < Ntransporters; i++)
    {
 		pthread_join(transporterThreads[i], NULL);
 		//WriteOutput(NULL, &transporters[i],NULL, NULL, TRANSPORTER_STOPPED); 
	}
	for (int i = 0; i < Nsmelters; i++)
    {
 		pthread_join(smelterThreads[i], NULL);
 		//WriteOutput(NULL, NULL,&smelters[i], NULL, SMELTER_STOPPED); 
	}
	for (int i = 0; i < NFoundries; i++)
    {
 		pthread_join(foundryThreads[i], NULL);
 		//WriteOutput( NULL, NULL, NULL,&foundries[i], FOUNDRY_STOPPED); 
	}

   // exit(0); 
	for (int i = 0; i < Nmines; i++)
	{
		sem_destroy(&mutexesMiner[i]);	
	}
	return 0;
#pragma endregion ENDSTAFF

	free(miners);
    free(mutexesMiner);
    free(canProduce);
    free(tmpminer);
    free(tmptransporter);
    free(tmpsmelter);
    free(tmpfoundry);
    free(transporters);
    free(smelters);
    free(mutexesSmelter);
    free(activeSmelters);
    free(activeSmelter_Controller);
    free(smelterProduced_Controller);
    free(unloaded_smelter);

}
