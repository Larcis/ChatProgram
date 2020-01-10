
#include "data_defines.h"
#pragma comment(lib,"ws2_32.lib")

int is;
void makeRequest(char*,SOCKET);
void catchUsers(SOCKET);
char menu(void);
int sendChoice(SOCKET);
void sendMessage(SOCKET);
void checkMessages(SOCKET);
void deleteMessage(SOCKET);
void messageHistory(SOCKET,int);
char* response(SOCKET);

int main(int argc , char *argv[])
{
    WSADATA wsa;
    //clients socket
    SOCKET s;
    //user s information
    PERSON me;
    //is used for store send and recv functions return values
    int i;
    //server address for connection
    struct sockaddr_in server;
    char buffer[256],choice=65;
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }
    printf("Initialised.\n");
    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }
    //set server s addres
    printf("Socket created.\n");
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT );
    //Connect to remote server
    if (connect(s , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("connect error");
        return 1;
    }
    puts("Connected");
    Sleep(1000);
	system("cls");
    printf("Please enter your id: ");
	scanf("%s",buffer);
	//send id to server
	is=send(s,buffer,strlen(buffer),0);
	if(is>0)
		printf("Controlling id...\n");
	else
	{
		printf("couldn t send data to server...\n");
		close(s);
		getch();	
		exit(1);
	}
	//take data from server	
	is=recv(s,buffer,255,0);
	if(is>0)
	{
		buffer[is]='\0';
		printf("Situation: %s\n",buffer);
		//if registered to this server
		if(!strcmp(buffer,"registered"))
		{
			printf("you are registered this server...\n");
			strcpy(buffer,"");
			//password control
			char password[6];
			getchar();
			printf("enter your password: ");
			scanf("%s",password);
			send(s,password,strlen(password),0);
			is=recv(s,buffer,255,0);
			buffer[is]='\0';
			if(!strcmp(buffer,"accessdenied"))
			{
				printf("wrong password!..\n");
				getch();
				exit(0);
			}
			//take your info from server
			is=recv(s,buffer,sizeof(PERSON),0);
			memcpy(&me,buffer,sizeof(PERSON));
			if(is>0)
				printf("\nNAME: %s PHONE: %s ID: %d PASSWORD: %s\n",me.name,me.phone,me.id,me.password);
			else
			{
				printf("couldn t get info from server...\n");
				getch();
				exit(1);
			}
		}
		//registered but connected to server from another socket
		else if(!strcmp(buffer,"opened"))
		{
			printf("You are registered and online this client shutting down...\n");
			getch();
			exit(1);
		}
		//if not registered
		else
		{
			//fill your info
			printf("enter your information...\n");
			getchar();
			printf("name: ");
			gets(me.name);
			printf("phone: ");
			gets(me.phone);
			int isvalid=0;
			for(i=0;i<30;i++)
			{
				me.flist[i]=-1;
			}
			// pasword must be 4-6 charecters and only alpha numeric charecters supported
			do
			{
				printf("password: ");
				scanf("%s",me.password);
				if(strlen(me.password)>6 || strlen(me.password)<4)
					printf("Password should be 4-6 charecter size!..\n");
				else
				{
					i=0;
					while(i<strlen(me.password) &&isalnum(me.password[i]))i++;
					if(i==strlen(me.password))
					{
						isvalid=1;
					}
					else{
						printf("Password must just contain alpha numeric charecters!..\n");
					}
				}		
			}while(!isvalid);
			me.fcount=0;
			//send your info to server
			is=send(s,(void*)&me,sizeof(PERSON),0);
			if(is>0)
				printf("Your info sended to server...\n");
			else
			{
				printf("Couldn t send your info to server...\n");
				getch();	
				exit(1);
			}
		}	
    }
	else
	{
		printf("Couldn t get answer from server...\n");
		getch();
		exit(1);
	}
	Sleep(500);	 
	system("cls");
	//menu
    while(choice!=(char)27)
    {
    	//clear buffer
    	strcpy(buffer,"");
    	switch(menu())
    	{
    		case '1':
    			system("cls");
    			printf("\n\t<<YOUT CONTACT LIST>>\n");
    			//make request
				makeRequest("/showcontacts",s);
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(!strcmp(buffer,"notempty"))
				{
					//recieve and output all coming user information
					catchUsers(s);
				}
				else
				{
					printf("\nYou don t have any friends pal. :(\n");
				}
    		break;
    		case '2':
    			system("cls");
    			printf("\n\t<<ADD CONTACT MENU>>\n");
				//make request
				makeRequest("/addcontact",s);
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(!strcmp(buffer,"notempty"))
				{
					//take and output data until stop flag
					catchUsers(s);
					//send your choice
					int ch=sendChoice(s);
					//take response from server
					char *result=response(s);
					if(!strcmp(result,"added"))
						printf("\n%d successfuly added.\n",ch);
					else
						printf("\nInvalid choice %d.\n",ch);
					//we dynamicly declared this data so when we finish
					//our job we must free it	
					free(result);
				}
				else
				{
					printf("\nThere isn t any friend suggestions for you pal!:)\n");
				}
				
    		break;
			
			case '3':
				system("cls");
    			printf("\n\t<<DELETE CONTACT MENU>>\n");
				//make request
    			makeRequest("/delcontact",s);
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(!strcmp(buffer,"notempty"))
				{
					//take and output data
					catchUsers(s);
					//send choice
					int ch=sendChoice(s);
					//take response
					char *result=response(s);
					if(!strcmp(result,"deleted"))
						printf("%d successfuly deleted.\n",ch);
					else
						printf("\nInvalid choice %d.\n",ch);
					free(result);
					
				}
				else
				{
					printf("\nYou don t have any friends pal. :(\n");
				}
    		break;
			case '4':
				system("cls");
    			printf("\n\t<<SEND A MESSAGE>>\n");
				sendMessage(s);
    		break;
			case '5':
				system("cls");
    			printf("\n\t<<INCOMING MESSAGES>>\n");
				checkMessages(s);
    		break;
    		case '6':
    			system("cls");
    			printf("\n\t<<CHAT HISTORY>>\n");
    			messageHistory(s,me.id);
    		break;
    		case '7':
    			system("cls");
    			printf("\n\t<<YOUR INFO>>\n");
    			//make request
    			makeRequest("/whoami",s);
				strcpy(buffer,"");
				//take response
				is=recv(s,buffer,sizeof(PERSON),0);
				memcpy(&me,buffer,sizeof(PERSON));
				//evaluate and output
				if(is>0)
					printf("\nNAME: %s \nPHONE: %s \nID: %d \nPASSWORD: %s\n",me.name,me.phone,me.id,me.password);
				else
				{
					printf("\ncouldn t get info from server...\n");
					closesocket(s);
					exit(1);
				}
				
    		break;
			case '8':
				system("cls");
    			printf("\n\t<<DELETE MESSAGE>>\n");
				deleteMessage(s);
			
			break;		
			case (char)27:
				printf("\nClient terminated normaly...\n");
				closesocket(s);
				exit(0);
			break;
			default:
				printf("\nunknown choice!\n"); 
			break;
					
		}
	    getch();
    	system("cls");
	}
	//closesocket(s);
	return 0;
}

void deleteMessage(SOCKET s)
{
	char buffer[256];
	makeRequest("/deletemessage",s);
	is=recv(s,buffer,255,0);
	buffer[is]='\0';
	if(is>0)
	{
		if(!strcmp(buffer,"opened"))
		{
			int count=0;
			//recv all sended messages from server 
			do
			{
				strcpy(buffer,"");
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(is!=5)
				{
					char id[5];
					char message[256];
					int i=0;
					while(buffer[i]!=';' && buffer[i]!='\0')
					{
						id[i]=buffer[i];
						i++;
					}
					id[i]='\0';
					strcpy(message,&buffer[i+1]);
					message[strlen(message)-2]='\0';
					printf("%d)[%s]>> %s\n",count+1,id,message);
					count++;
				}
			}while(is!=5);
			if(count!=0)
			{
				int ch;
				//send a choice which in 1 - count range
				do
				{
					printf("\nchoose one message for delete: ");
					scanf("%d",&ch);
				}while(ch<1 || ch>count);
				is=send(s,(void*)&ch,sizeof(int),0);
				if(is>0)
				{
					printf("%d. message deleted!.\n",ch);
				}
				else
				{
					printf("couldn t send info to server!..\n");
					closesocket(s);
					getch();
					exit(1);
				}
			}
			else
			{
				printf("you didn t send any messages.\n");
			}
			
		}
		else
		{
			printf("database issue!..\n");
		}
	}
	else
	{
		printf("couldn t get info from server!..\n");
		closesocket(s);
		getch();
		exit(1);
	}
}
void makeRequest(char* msg,SOCKET s)
{
	//send message to server
	is=send(s,msg,strlen(msg),0);
	//if you couldn t, server closed close client
	if(is<0)
	{
		printf("\nCouldn t made request! Server is OFFLINE\n");
		exit(1);
	}
}
void catchUsers(SOCKET s)
{
	PERSON newp;
	char buffer[256];
	printf("\nNAME%*cPHONE%*cUID\n",11,' ',10,' ');
	//until stop flag recv and output data (stop flag is 4 sized any string)
	do
	{
		is=recv(s,buffer,sizeof(PERSON),0);
		memcpy(&newp,buffer,sizeof(PERSON));
		if(is!=4)
			printf("%*s%*s%d\n",-16,newp.name,-16,newp.phone,newp.id);
		strcpy(buffer,"");
	}while(is!=4);
}
char menu(void)
{
	int i;
	//options
 	char *menu[]={"List Contacts","Add User","Delete User","Send Message","Check Messages","Message History","Whoami","Delete Message"};
	//output menu
	printf("\n\t\t--MENU--\n");
	for(i=0;i<8;i++)
		printf("\n\t%d)%s",i+1,menu[i]);
	printf("\n\t\tyour choice: ");
	//get user s choice
	return getch();
}
int sendChoice(SOCKET s)
{
	char ch[12];
	do
	{
		printf("\nChose one id from above: ");
		scanf("%s",ch);
	}while(atoi(ch)>30 || atoi(ch)<0 && ch[0]!=' ');
	//send your choice
	send(s,ch,strlen(ch),0);
	return atoi(ch);
}
char* response(SOCKET s)
{
	char *buffer;
	buffer=(char*)malloc(sizeof(char)*255);
	is=recv(s,buffer,255,0);
	buffer[is]='\0';
	return buffer;	
}
void sendMessage(SOCKET s)
{
	char buffer[255];
	//make request
	makeRequest("/sendmessage",s);
	//get formatted message until its size bigger than zero
	printf("\n\nformatted message(rec_id message)\nMessage: ");
	getchar();
	int valid;
	do
	{
		strcpy(buffer,"");
		scanf(" %[^\n]",buffer);	
		buffer[strlen(buffer)]='\0';
		//printf("buffer %s\n",buffer);
		int i=0;
		while(isdigit(buffer[i]) && buffer[i+1]!=' ' && buffer[i+1]!='\0' )i++;
		if(!isdigit(buffer[i]))
			valid=0;
		else 
			valid=1;
		if(strlen(buffer)<2 || buffer[0]==' ' || valid!=1)
			printf("\nMessage: ");	
	}while(strlen(buffer)<2 || buffer[0]==' ' || valid!=1);
	//printf("\nbuffer: %s \n",buffer);
	is=send(s,buffer,strlen(buffer),0);
	if(is>0)
		printf("\nMessage Sent\n");
	else
	{
		printf("\nCouldn't Sent Message !\n");
		exit(1);
	}
	//take response
    char *result=response(s);
	//evaluate
	if(!strcmp(result,"recieved"))
		printf("\nMessage Received\n");
		else if(!strcmp(result,"notrecieved"))
			printf("\nMessage Not Received\n");
	else
		printf("\nUser not in your contact list...\n");
	free(result);
}
void checkMessages(SOCKET s)
{
	char buffer[256];
	//make request
	makeRequest("/checkmessages",s);
	//take response and evaluate
	char* result=response(s);
	if(!strcmp(result,"failopen"))
		printf("\nYou don t have any messages!..\n");
	else
	{
		strcpy(buffer,"");
		is=recv(s,buffer,255,0);
		buffer[is]='\0';
		if(!strcmp(buffer,"notempty"))
		{
			//find suitable ones for showing
			int readable[30];
			int readable_c=0;
			do
			{
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(is!=5)
				{
					printf("%s \n",buffer);
					readable[readable_c]=atoi(&buffer[22]);
					//printf("%d\n",readable[readable_c]);
					readable_c++;
				}
			}while(is!=5);
			//send choice
			int ch=sendChoice(s);
			strcpy(buffer,"");
			int k=0;
			//control choice for is it valid
			while( k<readable_c && ch!=readable[k])k++;
			//if is it valid wait for message
			if(ch==readable[k] && readable_c!=k)
			{
				do
				{
					is=recv(s,buffer,255,0);
					buffer[is-2]='\0';
					if(is!=5)
						printf("%s\n",buffer);
				}while(is!=5);
			}
			else
				printf("\nInvalid choice!..\n");
			
		}
		else
		{
			printf("\nYou don t have any new messages!\n");
		}
	}
	free(result);
}
void messageHistory(SOCKET s,int id)
{
	char buffer[256];
	//make request
	makeRequest("/messagehistory",s);
	//take response and evaluate
	is=recv(s,buffer,255,0);
	buffer[is]='\0';
	if(!strcmp(buffer,"failopen"))
		printf("\nYou don t have any dialogue!..\n");
	else
	{
		strcpy(buffer,"");
		is=recv(s,buffer,255,0);
		buffer[is]='\0';
		if(!strcmp(buffer,"notempty"))
		{
			//find suitable ones for showing
			int readable[30];
			int readable_c=0;
			do
			{
				is=recv(s,buffer,255,0);
				buffer[is]='\0';
				if(is!=5)
				{
					printf("%s \n",buffer);
					readable[readable_c]=atoi(&buffer[25]);
					readable_c++;
				}
			}while(is!=5);
			//send choice
			int ch=sendChoice(s);
			int a = ch;
			char* result=response(s);
			if(!strcmp(result,"error1"))
			{
				printf("\nInvalid choice!..\n");
				
			}
			else
			{
				char* result=response(s);
				Sleep(200);
				if(!strcmp(result,"noterror"))
				{
					//if your choice valid
					do
					{
						strcpy(buffer,"");
						is=recv(s,buffer,255,0);
						buffer[is]='\0';
						//take messages
						if(is!=5)
						{
							char c=buffer[strlen(buffer)-2];
							printf("%c");
							buffer[strlen(buffer)-2]='\0';
							//evaluate messages and output "+" message readed "-" message not readed
							if(c=='-')
							{
								//system("color 06");
								printf("\n [%d]>> %s\n",a,buffer);
								//system("color 0f");
							}
							else if(c=='+')
							{
								//system("color 0a");
								printf("\n+[%d]>> %s\n",id,buffer);								
								//system("color 0f");
							}
							else
							{
								//system("color 0c");
								printf("\n-[%d]>> %s\n",id,buffer);	
								//system("color 0f");
							}
						}
					}while(is!=5);
				}
					
			}
			free(result);
			
		}
		else
		{
			printf("\nyou dont have any dialog!\n");
		}
			
	}
}
