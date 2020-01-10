
#include "data_defines.h"
#pragma comment(lib, "ws2_32.lib") 


PERSON  registered[max_clients];
int registered_c=0;
pthread_t th[max_clients],handle[max_clients];
pthread_mutex_t mtx;

void messageHistory(PERSON*);
void deleteContact(PERSON*);
void checkMessages(PERSON*);
void sendMessage(PERSON*);
void deleteMessage(PERSON*);
void addContact(PERSON*);
void saveCurClients(void);
//thread functions
void* requestFunc(void*);
void* registerClient(void*);
int main(int argc , char *argv[])
{
    WSADATA wsa;
    int control;
    //server s socket and new client s socket
    SOCKET master , new_socket;
    //file for save registered clients
    FILE* clients;
    //path for clients file
    char client_path[200];
	strcpy(client_path,DATABASE);
    strcat(client_path,"clients.txt");
    //address variables
    struct sockaddr_in server, address;
    int  addrlen, i;
    printf("debug");
    //take saved clients from file in byte mode and output them information
	clients=fopen(client_path,"r+b");
	if(!clients)
		printf("couldn t find registered clients file\n");
	else
	{
		printf("\t--REGISTERED CLIENTS--\n");
		printf("\nNAME%*cPHONE%*cUID\n",11,' ',10,' ');
		while(fread(&registered[registered_c],sizeof(PERSON),1,clients))
			{
				registered[registered_c].sock=0;
				printf("%*s%*s%d\n",-16,registered[registered_c].name,-16,registered[registered_c].phone,registered[registered_c].id);
				/*printf("name: %s phone: %s id: %d socket: %d\n",registered[registered_c].name,
																registered[registered_c].phone,
																registered[registered_c].id,
																registered[registered_c].sock);*/
				registered_c++;
			}
		fclose(clients);	
	}
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised.\n");
     
    //Create a socket for server
    if((master = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Socket created.\n");
     
    //Prepare the sockaddr_in structure for server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons( PORT );
     
    //Bind server to master socket with server address
    if( bind(master ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    puts("Bind done");
    //Listen to incoming connections ,with most 5 queue
    listen(master , max_clients);
    Sleep(1500);
	system("cls"); 
    puts("Waiting for incoming connections...");
     
    addrlen = sizeof(struct sockaddr_in);
    //server started 
    while(TRUE)
    {
    	Sleep(250);
		new_socket = accept(master , (struct sockaddr *)&address, (int *)&addrlen);
		//if a socket call connect on us server and there is enough space for it
        if (new_socket>0 && registered_c<max_clients) 
        {
        	//look for an empty handle thread type
        	i=0;
        	while(i<max_clients && handle[i]!=0)i++;
        	HANDLERARG arg[max_clients];
        	pthread_mutex_lock(&mtx);
			arg[i].i=i;
        	arg[i].s=new_socket;
        	pthread_mutex_unlock(&mtx);
			//start register operation 
          	control=pthread_create(&handle[i],NULL,registerClient,(void*)&arg[i]);
          	if(control<0)
          		printf("couldn t create handle thread!..\n");
          	else
			  	printf("handle thread created!..\n");	
        }
        //control for is server full?
        else if(registered_c==max_clients)
        {
        	printf("SERVER IN FULL CAPACITY !! \n");
        	getchar();
		}
    }
    //meaningless 
    closesocket(master);
    WSACleanup();
     
    return 0;
}

void* registerClient(void* arg)
{
	//cast arguments for use
	HANDLERARG *me=(HANDLERARG*)arg;
	printf("handling socket : %d  handle thread id : %d  \n",me->s,me->i);
	PERSON newp;
	int i;
	char new_idc[12];
	//take an id from client
	int control=recv(me->s,new_idc,12,0);
	new_idc[control]='\0';
  	if(control>0)
  	{
  		int new_id=atoi(new_idc);
  		printf("connection request with %d id...\n",new_id);
		//look for is this id in registered clients
		i=0;
		pthread_mutex_lock(&mtx);
		while(i<registered_c && new_id!=registered[i].id)i++;
		pthread_mutex_unlock(&mtx);
		//if is it so
		if( registered[i].id==new_id && new_id<registered_c && new_id>=0 && i!=registered_c && registered_c!=0)
		{
			//control for the clients socket is open or not
			if(registered[i].sock==0)
			{
				//if socket is closed , say client u r registered and offline
				char *mess="registered";
				newp=registered[i];
			    control = send(me->s,mess,strlen(mess),0);
				if(control>0)
					printf("control message sended!..\n");
				else
					printf("couldn t send control message!..\n");
				//take password from client and control it	
				char password[6];
				control=recv(me->s,password,6,0);
				password[control]='\0';
				//printf("password %s\n",password);
				if(!strcmp(password,registered[i].password))
				{
					char *mess="accessgranted";
					send(me->s,mess,strlen(mess),0);
					//inform client about itself
					//use mutex for writing global variables 
					pthread_mutex_lock(&mtx);	
					control=send(me->s,(void*)&registered[i],sizeof(PERSON),0);
					pthread_mutex_unlock(&mtx);	
					if(control>0)
						printf("person info message sended!..\n");
					else
						printf("couldn t send person info message!..\n");
					//set registered client s socket to current socket	
					pthread_mutex_lock(&mtx);	
					registered[i].sock=me->s;
					//start request func for current client
					int control=pthread_create(&th[i],NULL,requestFunc,(void*)&registered[i]);
					pthread_mutex_unlock(&mtx);	
					if(control<0)
						printf("couldn t create thread  for %s\n",registered[i].name);
					else
						printf("thread created succesfully for %s\n",registered[i].name);
					printf("\t...CONNECTED... \n\tsocket: %d \n\tname: %s \n\tphone: %s \n\tid: %d \n\t...CONNECTED...\n" , me->s , newp.name ,newp.phone,newp.id);
				}
				else
				{
					char *mess="accessdenied";
					send(me->s,mess,strlen(mess),0);
					printf("Wrong password from %d\n",new_id);
				}	
						
			}
			else
			{
				//if client registered but its socket open too then send close request to client
				char* mess="opened";
				control = send(me->s,mess,strlen(mess),0);
				if(control>0)
					printf("control message sended!..\n");
				else
					printf("couldn t send control message!..\n");
			}
		}
		else
		{
			//if client doesn t registered to this server
			char *mess="unregistered";
			control = send(me->s,mess,strlen(mess),0);
			if(control>0)
				printf("control message sended!..\n");
			else
				printf("couldn t send control message!..\n");
			//take client s info
			char buffer[256];	
			control=recv(me->s,buffer,sizeof(PERSON),0);
			if(control==sizeof(PERSON))
			{
				//set it as server s regitered_c th user.
				pthread_mutex_lock(&mtx);
				memcpy(&registered[registered_c],buffer,sizeof(PERSON));
				registered[registered_c].sock=me->s;
				registered[registered_c].id=registered_c;
				registered_c++;
				pthread_mutex_unlock(&mtx);
				char id[10];
				itoa(registered_c-1,id,10);
				strcat(id,".txt");
				char filename[]=DATABASE;
				strcat(filename,id);
				pthread_mutex_lock(&mtx);
				FILE* file=fopen(filename,"w+");
				if(file)
					fclose(file);
				pthread_mutex_unlock(&mtx);
				printf("person info message recieved!..\n");
				pthread_mutex_lock(&mtx);
				//add client info to server and start request func  	
				newp=registered[registered_c-1];
				control=pthread_create(&th[registered_c-1],NULL,requestFunc,(void*)&registered[registered_c-1]);
				pthread_mutex_unlock(&mtx);
				if(control<0)
					printf("couldn t create thread  for %s\n",registered[registered_c-1].name);
				else
					printf("thread created succesfull for %s\n",registered[registered_c-1].name);
				saveCurClients();
				printf("\t...CONNECTED... \n\tsocket: %d \n\tname: %s \n\tphone: %s \n\tid: %d \n\t...CONNECTED...\n" , me->s , newp.name ,newp.phone,newp.id);	
			}
			else
			{
				printf("couldn t recieve person info message!..\n");
			}
		}
	}
	//set this handle thread as free
	pthread_mutex_lock(&mtx);	
	handle[me->i]=0;
	printf("handle thread finished thid: %d !..\n",me->i);
	pthread_exit(&handle[me->i]);
	pthread_mutex_unlock(&mtx);	
}

void saveCurClients()
{
	//update clients file and out all registered clients to server screen
	char filename[]=DATABASE;
	int i;
	pthread_mutex_lock(&mtx);	
    strcat(filename,"clients.txt");
    FILE *clients=fopen(filename,"w+b");
	if(!clients)
		printf("server couldn t save current clients to file \n");
	else
	{
		printf("\nREGISTERED CLIENTS\n");
		printf("current number of registered clients: %d \n",registered_c);
		for(i=0;i<registered_c;i++)
		{
			fwrite(&registered[i],sizeof(PERSON),1,clients);
			printf("name: %s phone: %s id: %d pass: %s\n",registered[i].name,registered[i].phone,registered[i].id,registered[i].password);
		}
			
		fclose(clients);
		printf("current clients saved to the file\n");		
	}
	pthread_mutex_unlock(&mtx);
}
void addContact(PERSON* me)
{
	int validChoices[max_clients]={-1};
	int count=0;
	int i,j,is;
	//look all registered clients and pick fit ones
	//(-not in my contactlist ,-not me) 
	for(i=0;i<registered_c;i++)
	{
		j=0;
		while(j<me->fcount && i!=me->flist[j])j++;
		if(i!=me->flist[j] && i!=me->id)
		{
			validChoices[count]=i;
			count++;
		}
	}
	if(count>0)
	{
		char *msg="notempty";
		send(me->sock,msg,strlen(msg),0);
		Sleep(100);
		//send valid ones to client
		for(i=0;i<count;i++)
			send(me->sock,(void*)&registered[validChoices[i]],sizeof(PERSON),0);
			Sleep(150);
		//inform client about server finished sending with a finish sign
		char *finish="....";
		is = send(me->sock,finish,4,0);
		Sleep(100);	
		//take clients choice for adding
		int choice;
		char ch[12];
		is=recv(me->sock,ch,12,0);
		ch[is]='\0';
		choice=atoi(ch);
	    //look is it valid
	    j=0;
		while(j<me->fcount && me->flist[j]!=choice)j++;
		if(me->flist[j]==choice)
			is=-1;
		j=0;
		while(j<count && choice!=validChoices[j])j++;	
		if(is>0 && choice==validChoices[j] && choice!=me->id && j!=count)
		{
			//add clients list if its choice valid and inform client
			pthread_mutex_lock(&mtx);
			me->flist[me->fcount]=choice;
			me->fcount++;
			registered[choice].flist[registered[choice].fcount]=me->id;
			registered[choice].fcount++;
			pthread_mutex_unlock(&mtx);
			char *mess="added";
			send(me->sock,mess,strlen(mess),0);
			saveCurClients();
		}
		else
		{
			//if choice is invalid inform client about that
			char *mess="notadded";
			send(me->sock,mess,strlen(mess),0);
		}
	}
	else
	{
		char *msg="empty";
		send(me->sock,msg,strlen(msg),0);
		Sleep(100);
	}
	
}

void deleteContact(PERSON *me)
{
	int i,is;
	//if clients contact list not emty
	if(me->fcount>0)
	{
		char *mess="notempty";
		send(me->sock,mess,strlen(mess),0);
		Sleep(100);
		//sent all contact list to client
		for(i=0;i<me->fcount;i++)
		{
			send(me->sock,(void*)&registered[me->flist[i]],sizeof(PERSON),0);
		}
		Sleep(150);
		//finish flag
		char *finish="....";
	    send(me->sock,finish,4,0);
	    Sleep(100);
		int choice,j=0;
		char ch[12];
		//take clients choice
		is=recv(me->sock,ch,12,0);
		ch[is]='\0';
		choice=atoi(ch);
		//and choice in it, delete choiced element
		while(j<me->fcount && me->flist[j]!=choice)j++;
		if(me->flist[j]==choice)
		{
			//me->id choice, delete message history
			pthread_mutex_lock(&mtx);
			char filename[]=DATABASE;
			char tmp_file[]=DATABASE;
			strcat(tmp_file,"tmp.txt");
			char id[10];
			itoa(me->id,id,10);
			strcat(id,".txt");
			strcat(filename,id);
			FILE *file,*tmp=fopen(tmp_file,"w+");
			//set filenames  me->id.txt , tmp.txt
			//and delete lines with choice id
			if(tmp)
			{
				if(file=fopen(filename,"r"))
				{
					char line[256];
					while(fgets(line,256,file))
					{
						int k=0;
						while(line[k]!=';' && line[k]!='\0')
						{
							id[k]=line[k];
							k++;
						}
						id[k]='\0';
						if(atoi(id)!=choice)
							fputs(line,tmp);
					}
					fclose(file);
				}
				fclose(tmp);
				remove(filename);
				rename(tmp_file,filename);
			}
			strcpy(filename,DATABASE);
			strcpy(id,"");
			itoa(choice,id,10);
			strcat(id,".txt");
			strcat(filename,id);
			tmp=fopen(tmp_file,"w+");
			//set filenames  choice.txt , tmp.txt
			//and delete lines with me->id
			if(tmp)
			{
				if(file=fopen(filename,"r"))
				{
					char line[256];
					while(fgets(line,256,file))
					{
						int k=0;
						while(line[k]!=';' && line[k]!='\0')
						{
							id[k]=line[k];
							k++;
						}
						id[k]='\0';
						if(atoi(id)!=me->id)
							fputs(line,tmp);
					}
					fclose(file);
				}
				fclose(tmp);
				remove(filename);
				rename(tmp_file,filename);
			}
			//shift me->id s frientlist 
			for(i=j;i<me->fcount;i++)
				me->flist[i]=me->flist[i+1];
			me->fcount--;
			int k=0;
			//find me->id from choice s friendlist
			while(registered[choice].fcount>k &&registered[choice].flist[k]!=me->id)k++;
			if(registered[choice].flist[k]==me->id)
			{
				//shift choice s frientlist 
				for(i=k;i<registered[choice].fcount;i++)
					registered[choice].flist[i]=registered[choice].flist[i+1];	
				registered[choice].fcount--;
			}
			pthread_mutex_unlock(&mtx);
			//inform client
			char *mess="deleted";
			send(me->sock,mess,strlen(mess),0);
			saveCurClients();
		}
		else
		{
			//inform client about invalid choice
			char *mess="notdeleted";
			send(me->sock,mess,strlen(mess),0);
		}
	}
	else
	{
		//inform client about its contact list
		//printf("there isnt anybody to delete for %s\n",me->name);
		char *mess="empty";
		send(me->sock,mess,strlen(mess),0);
		Sleep(100);
	}
	
}

void sendMessage(PERSON * me)
{
	char buffer[256];
	int is,j,i;
	//take formatted message form client
	is=recv(me->sock,buffer,255,0);
	buffer[is]='\0';
	char path[]=DATABASE;
	char id[16],mess[255];
	i=0;j=0;
	//extract id part from formatted message
	while(buffer[i]!=' ' && buffer[i]!='\0')
	{
		id[i]=buffer[i];
		i++;
	}
	id[i]='\0';
	char rec_id[12];
	strcpy(rec_id,id);
	i++;
	//extract message part from formatted message
	while(buffer[i]!='\0')
	{
		mess[j]=buffer[i];
		i++;
		j++;
	}
	mess[j]='\0';
	j=0;
	//look contact list for control reciever id
	while(j<me->fcount && me->flist[j]!=atoi(id))j++;
	/*printf("id: %s atoi: %d\n",id,atoi(id));
	for(i=0;i<me->fcount;i++)
		printf("%d ",me->flist[i]);
	printf("\n");*/	
	if(me->flist[j]==atoi(id))
	{
		//open id s message file
		strcat(id,".txt");
		strcat(path,id);
		//printf("path: %s\n",path);
		pthread_mutex_lock(&mtx);
		FILE* fo=fopen(path,"a");
		if(!fo)
		{
			//inform client
			char* msg="notrecieved";
			send(me->sock,msg,strlen(msg),0);
		}
		else
		{
			//append message to reciever s file with sender id and put '*' at end
			strcat(mess,"*");
			fprintf(fo,"%d;%s\n",me->id,mess);
			//inform client
			char* msg="recieved";
			send(me->sock,msg,strlen(msg),0);
			fclose(fo);	
		}
		//write same message my file with a '-' at end
		char me_id[12];
		strcpy(path,DATABASE);
		itoa(me->id,me_id,10);
		strcat(me_id,".txt");
		strcat(path,me_id);
		fo=fopen(path,"a");
		mess[strlen(mess)-1]='-';
		if(fo)
		{
			fprintf(fo,"%d;%s\n",atoi(rec_id),mess);
			fclose(fo);
		}
		pthread_mutex_unlock(&mtx);	
	}
	else
	{
		//inform client
		char* mess="notincontactlist";
		send(me->sock,mess,strlen(mess),0);
	}
}
void checkMessages(PERSON* me)
{
	int i;
	//open client s own message file
	FILE *messages;
	char fname[4],filename[]=DATABASE;
	itoa(me->id,fname,10);
	strcat(fname,".txt");
	strcat(filename,fname);
	pthread_mutex_lock(&mtx);
	messages=fopen(filename,"r+");
	if(!messages)
	{
		//inform client
		char *mess="failopen";
		send(me->sock,mess,strlen(mess),0);
	}
	else
	{
		//inform client
		char *mess="opened";
		send(me->sock,mess,strlen(mess),0);
		Sleep(20);
		char id[16],line[256];
		int k,count=0;
		int readable[max_clients];
		int readable_c=0;
		for(i=0;i<max_clients;i++)
		{
			readable[i]=-1;
		}
		//read line by line from file
		while(fgets(line,255,messages))
		{
			k=0;
			while(line[k]!=';' && line[k]!='\0')
			{
				id[k]=line[k];
				k++;	
			}
			id[k]='\0';
			//extract id and make this id readable
			k=0;
			//am i add this id before?
			while(k<readable_c && readable[k]!=atoi(id))k++;
			if(readable[k]!=atoi(id) && line[strlen(line)-2]=='*')
			{
				//no definetly not
				 readable[readable_c]=atoi(id);
				 readable_c++;
			}
		}
		fclose(messages);
		pthread_mutex_unlock(&mtx);
		if(readable_c>0)
		{
			char *msg="notempty";
			send(me->sock,msg,strlen(msg),0);
			Sleep(200);
			//send readable ids to client
			for(k=0;k<readable_c;k++)
			{
			    char messs[]="you have messages from    ";
				itoa(readable[k],&messs[23],10);//this row import id to end of above string	
				//send above string (with id added)	
				send(me->sock,messs,strlen(messs),0);
				Sleep(150);
			}
			Sleep(200);
			//stop flag
			char *m="bitti";
			send(me->sock,m,strlen(m),0);
			int choice;
			char ch[12];
			//take choice from client
			k=recv(me->sock,ch,12,0);
			ch[k]='\0';
			choice=atoi(ch);
			//printf("choice %d\n",choice);
			k=0;
			//look is choice valid
			while( k<readable_c && choice!=readable[k])k++;
			//if it valid send requested messages
			if(choice==readable[k] && readable_c!=k)
			{
				//go to the files begining then send all messages with choosen id
				messages=fopen(filename,"r+");
				while(fgets(line,255,messages))
				{
					k=0;
					while(line[k]!=';' && line[k]!='\0')
					{
						id[k]=line[k];
						k++;	
					}
					if(atoi(id)==choice && line[strlen(line)-2]=='*')
					{
						k++;
						fseek(messages,-3,SEEK_CUR);
						fputs("+",messages);
						fseek(messages,2,SEEK_CUR);
						send(me->sock,&line[k],strlen(&line[k]),0);
						Sleep(300);
					}	
				}
				fclose(messages);
				Sleep(200);
				//stop flag
				char *m="bitti";
				send(me->sock,m,strlen(m),0);
			}
		}
		else
		{
			fclose(messages);
			char *msg="empty";
			send(me->sock,msg,strlen(msg),0);
			Sleep(200);
		}
	}
}
void messageHistory(PERSON* me)
{
	int i;
	//open client s own message file
	FILE *messages;
	char fname[4],filename[]=DATABASE;
	itoa(me->id,fname,10);
	strcat(fname,".txt");
	strcat(filename,fname);
	pthread_mutex_lock(&mtx);
	messages=fopen(filename,"r+");
	if(!messages)
	{
		//printf("coulnd t open file!..\n");
		//inform client
		char *mess="failopen";
		send(me->sock,mess,strlen(mess),0);
	}
	else
	{
		//inform client
		char *mess="opened";
		send(me->sock,mess,strlen(mess),0);
		Sleep(20);
		char id[16],line[256];
		int k,count=0;
		int readable[max_clients];
		int readable_c=0;
		for(i=0;i<max_clients;i++)
		{
			readable[i]=-1;
		}
		//read line by line from file
		while(fgets(line,255,messages))
		{
			k=0;
			while(line[k]!=';' && line[k]!='\0')
			{
				id[k]=line[k];
				k++;	
			}
			//extract id and make this id readable
			k=0;
			//am i add this id before?
			while(k<readable_c && readable[k]!=atoi(id))k++;
			if(readable[k]!=atoi(id))
			{
				//no definetly not
				 readable[readable_c]=atoi(id);
				 readable_c++;
			}
		}
		if(readable_c>0)
		{
			char *msg="notempty";
			send(me->sock,msg,strlen(msg),0);
			Sleep(200);
			//send readable ids to client
			for(k=0;k<readable_c;k++)
			{
			    char messs[]="you have a dialogue with      ";
				itoa(readable[k],&messs[25],10);//this row import id to end of above string	
				//send above string (with id added)	
				send(me->sock,messs,strlen(messs),0);
				Sleep(300);
			}
			Sleep(500);
			//stop flag
			char *m="bitti";
			send(me->sock,m,strlen(m),0);
			int choice;
			char ch[12];
			//take choice from client
			k=recv(me->sock,ch,12,0);
			ch[k]='\0';
			choice=atoi(ch);
			k=0;
			//look is choice valid
			while( k<readable_c && choice!=readable[k])k++;
			//if it valid send requested messages
			if(choice==readable[k] && readable_c!=k)
			{
				char* msg="noterror1";
				send(me->sock,msg,strlen(msg),0);
				fclose(messages);
				messages=fopen(filename,"r+");
				while(fgets(line,255,messages))
				{
					k=0;
					while(line[k]!=';' && line[k]!='\0')
					{
						id[k]=line[k];
						k++;	
					}
					//extract id and control and make unreaded messages readed
					if(choice==atoi(id) && line[strlen(line)-2]=='*')
					{
						//printf("line: %s\n",line);
						fseek(messages,-3,SEEK_CUR);
						fputs("+",messages);
						fseek(messages,2,SEEK_CUR);
					}
				}
				fclose(messages);
				strcpy(filename,DATABASE);
				itoa(choice,id,10);
				strcat(id,".txt");
				strcat(filename,id);
				messages=fopen(filename,"r");
				//fseek(messages,0,SEEK_SET);
				if(!messages)
				{
					char* msg="error";
					send(me->sock,msg,strlen(msg),0);
				}
				else
				{
					strcpy(line,"");
					char* msg="noterror";
					send(me->sock,msg,strlen(msg),0);
					while(fgets(line,255,messages))
					{
						k=0;
						while(line[k]!=';' && line[k]!='\0')
						{
							id[k]=line[k];
							k++;	
						}
						if(atoi(id)==me->id)
						{
							k++;
							//printf("%s\n",&line[k]);
							send(me->sock,&line[k],strlen(&line[k]),0);
						}
						Sleep(300);	
					}
					fclose(messages);
					Sleep(200);
					//stop flag
					char *m="bitti";
					send(me->sock,m,strlen(m),0);
				}
			}
			else
			{
				fclose(messages);
				char* msg="error1";
				send(me->sock,msg,strlen(msg),0);
			}
			
		}
		else
		{
			fclose(messages);
			char *msg="empty";
			send(me->sock,msg,strlen(msg),0);
			Sleep(200);
		}
	}
	pthread_mutex_unlock(&mtx);
}
void deleteMessage(PERSON* me)
{
	FILE* file;
	char filename[]=DATABASE;
	char id[12];
	itoa(me->id,id,10);
	strcat(id,".txt");
	strcat(filename,id);
	file=fopen(filename,"r");
	//open me->id file
	if(file)
	{
		char *msg="opened";
		send(me->sock,msg,strlen(msg),0);
		Sleep(200);
		char line[256];
		char delmessage[256];
		int count=0;
		//send all lines finishs with '-' to client respectivly
		while(fgets(line,255,file))
		{
			if(line[strlen(line)-2]=='-')
			{
				send(me->sock,line,strlen(line),0);
				count++;
			}
			Sleep(300);
		}
		fclose(file);
		Sleep(300);
		char *flag=".....";
		send(me->sock,flag,strlen(flag),0);
		//if there is more than 0 lines
		if(count>0)
		{
			int ch;
			int is=recv(me->sock,(void*)&ch,sizeof(int),0);
			//printf("choice for delete message: %d\n",ch);
			if(is>0)
			{
				count=0;
				file=fopen(filename,"r");
				char tmpFname[]=DATABASE;
				strcat(tmpFname,"tmp.txt");
				FILE *tmp=fopen(tmpFname,"w+");
				//delete choosen line from my file
				if(tmp && file)
				{
					strcpy(line,"");
					while(fgets(line,255,file))
					{
						if(line[strlen(line)-2]=='-')
						{
							count++;
						}
						if(count!=ch || line[strlen(line)-2]!='-')
							fputs(line,tmp);
						else
						{
							int i=0;
							while(line[i]!=';' && line[i]!='\0')
							{
								id[i]=line[i];
								i++;
							}
							id[i]='\0';
							strcpy(delmessage,&line[i]);
							delmessage[strlen(delmessage)-2]='\0';
							//printf("del message: %s\n",delmessage);
						}
						strcpy(line,"");	
					}
					fclose(tmp);
					fclose(file);
					remove(filename);
					rename(tmpFname,filename);
				}
				else
				{
					if(tmp)
						fclose(tmp);
					if(file)
						fclose(file);	
				}
				strcpy(filename,DATABASE);
				strcat(id,".txt");
				strcat(filename,id);
				file=fopen(filename,"r");
				tmp=fopen(tmpFname,"w+");
				int deleted=0;
				//delete choosen file from choosen id s file
				if(tmp && file)
				{
					strcpy(line,"");
					char message[256];
					while(fgets(line,255,file))
					{
						int i=0;
						while(line[i]!=';' && line[i]!='\0')i++;
						strcpy(message,&line[i]);
						message[strlen(message)-2]='\0';
						if(strcmp(delmessage,message) || deleted)					
							fputs(line,tmp);
						else
						{
							//printf("%s deleted from %s \n",line,filename);
							deleted=1;	
						}
						strcpy(line,"");	
					}
					fclose(tmp);
					fclose(file);
					remove(filename);
					rename(tmpFname,filename);
				}
				else
				{
					if(tmp)
						fclose(tmp);
					if(file)
						fclose(file);	
				}
			}
			else
			{
				printf("couldn t get answer from %d:%s",me->id,me->name);
			}
		}
		
		
	}
	else
	{
		char *msg="filenotopened";
		send(me->sock,msg,strlen(msg),0);
		Sleep(200);
	}
}
void* requestFunc(void* args)
{
	//cast arguments
	PERSON *me=(PERSON*)args;
	int is,i,j;
	char buffer[256];
	//always wait a request from client
	while((is=recv(me->sock,buffer,255,0))>0)
	{
		//recv do not put \0 to end of string so do it manually
		buffer[is]='\0';
		//if clients request /whoami send clients information to it
		if(!strcmp(buffer,"/whoami"))
			send(me->sock,(void*)me,sizeof(PERSON),0);
		//add request		
		else if(!strcmp(buffer,"/addcontact"))
		{
			addContact(me);
		}
		//delete from contact list
		else if(!strcmp(buffer,"/delcontact"))
		{
			deleteContact(me);
		}
		//show all contacts
		else if(!strcmp(buffer,"/showcontacts"))
		{
			if(me->fcount>0)
			{
				char *msg="notempty";
				send(me->sock,msg,strlen(msg),0);
				Sleep(100);
				//send all list to client
				for(i=0;i<me->fcount;i++)
				{
					send(me->sock,(void*)&registered[me->flist[i]],sizeof(PERSON),0);
				}
				Sleep(100);
				//stop flag
				char *finish="....";
				is = send(me->sock,finish,4,0);
				
			}
			else 
			{
				char *msg="empty";
				send(me->sock,msg,strlen(msg),0);
				Sleep(100);
			}
		}
		//sending message to someone
		else if(!strcmp(buffer,"/sendmessage"))
		{
			sendMessage(me);
		}
		//show message request
		else if(!strcmp(buffer,"/checkmessages"))
		{
			checkMessages(me);
		}
		else if(!strcmp(buffer,"/messagehistory"))
		{
			messageHistory(me);
		}
		else if(!strcmp(buffer,"/deletemessage"))
		{
			deleteMessage(me);
		}
		else
		{
			printf("Invalid request from %d:%s\n",me->id,me->name);
		}
		//write clients request to server screen
		pthread_mutex_lock(&mtx);
		printf("[id:%d name:%s]: %s \n",me->id,me->name,buffer);
		pthread_mutex_unlock(&mtx);
		strcpy(buffer,"");
	}
	//if somethind unexpected happened
	if(is==SOCKET_ERROR)
	{
		printf("\t...DISCONNECTED_UNEXPECTEDLY... \n\tsocket: %d \n\tname: %s \n\tphone: %s \n\tid: %d \n\t...DISCONNECTED_UNEXPECTEDLY...\n" , me->sock , me->name ,me->phone,me->id);	
	}
	//if client disconnect as valid
	else
	{
		printf("\t...DISCONNECTED... \n\tsocket: %d \n\tname: %s \n\tphone: %s \n\tid: %d \n\t...DISCONNECTED...\n" , me->sock , me->name ,me->phone,me->id);	
	}
	//set this socket as empty
	close(me->sock);
	pthread_mutex_lock(&mtx);
	registered[me->id].sock=0;
	th[me->id]=0;
	pthread_exit(&th[me->id]);
	pthread_mutex_unlock(&mtx);
} 
