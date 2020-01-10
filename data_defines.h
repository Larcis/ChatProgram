#ifndef INCLUDES
#define INCLUDES

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<winsock2.h>

//data defines
#define DATABASE ""
#define max_clients 30 
#define PORT 8888

//user type
typedef struct{
	char name[32];
	char phone[32];
	char password[6];
	int  flist[30];
	int fcount;
	int id;
	SOCKET sock;
}PERSON;

typedef struct
{
	int i;
	SOCKET s;
}HANDLERARG;
#endif
