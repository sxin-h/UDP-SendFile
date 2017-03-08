// UDP-SendClient.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


/*************************************************************************
> File Name: client.c
> Author: ljh
************************************************************************/
#include<sys/types.h> 

#include<stdio.h> 
#include<stdlib.h> 
#include<errno.h> 

#include<stdarg.h> 
#include<string.h> 
#include <WinSock2.h>


#define SERVER_PORT 8000 
#define BUFFER_SIZE 500 
#define FILE_NAME_MAX_SIZE 512 

#pragma comment(lib,"ws2_32.lib")

/* ��ͷ */
typedef struct
{
	long int id;
	int buf_size;
	unsigned int  crc32val;   //ÿһ��buffer��crc32ֵ
	int errorflag;
}PackInfo;

/* ���հ� */
struct RecvPack
{
	PackInfo head;
	char buf[BUFFER_SIZE];
} data;


//----------------------crc32----------------
static unsigned int crc_table[256];
static void init_crc_table(void);
static unsigned int crc32(unsigned int crc,  char * buffer, unsigned int size);
/* ��һ�δ����ֵ��Ҫ�̶�,������Ͷ�ʹ�ø�ֵ����crcУ����, ��ô���ն�Ҳͬ����Ҫʹ�ø�ֵ���м��� */
unsigned int crc = 0xffffffff;

/*
* ��ʼ��crc��,����32λ��С��crc��
* Ҳ����ֱ�Ӷ����crc��,ֱ�Ӳ��,
* ���ܹ���256��,�����ۻ�,�����ɵıȽϷ���.
*/
static void init_crc_table(void)
{
	unsigned int c;
	unsigned int i, j;

	for (i = 0; i < 256; i++)
	{
		c = (unsigned int)i;

		for (j = 0; j < 8; j++)
		{
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}

		crc_table[i] = c;
	}
}


/* ����buffer��crcУ���� */
static unsigned int crc32(unsigned int crc,  char *buffer, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++)
	{
		crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	}

	return crc;
}

//���������  
int main()
{

	WORD request;
	WSADATA ws;
	request = MAKEWORD(2, 2);
	int err = WSAStartup(request, &ws);
	if (err != 0)
	{
		return 1;
	}
	if (LOBYTE(ws.wVersion) != 2 || HIBYTE(ws.wVersion) != 2)
	{
		WSACleanup();
		return 1;
	}


	long int id = 1;
	unsigned int crc32tmp;

	/* ����˵�ַ */
	struct sockaddr_in server_addr;
	//bzero(&server_addr, sizeof(server_addr));
	memset(&server_addr, 0x00,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(SERVER_PORT);
	int server_addr_length = sizeof(server_addr);

	/* ����socket */
	SOCKET client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_socket_fd < 0)
	{
		printf("Create Socket Failed:");
		exit(1);
	}
	//crc32
	init_crc_table();

	/* �����ļ����������� */
	char file_name[FILE_NAME_MAX_SIZE + 1];
	//bzero(file_name, FILE_NAME_MAX_SIZE + 1);
	memset(file_name, 0x00, FILE_NAME_MAX_SIZE + 1);
	printf("Please Input File Name On Server: ");
	scanf("%s", file_name);

	char buffer[BUFFER_SIZE];
	//bzero(buffer, BUFFER_SIZE);
	memset(buffer, 0x00, BUFFER_SIZE);

	strncpy(buffer, file_name, strlen(file_name)>BUFFER_SIZE ? BUFFER_SIZE : strlen(file_name));

	/* �����ļ��� */
	if (sendto(client_socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
	{
		printf("Send File Name Failed:");
		exit(1);
	}

	/* ���ļ���׼��д�� */


	
	FILE *fp = fopen("D:\\MyDB.accdb", "w");
	if (NULL == fp)
	{
		printf("File:\t%s Can Not Open To Write\n", file_name);
		exit(1);
	}

	/* �ӷ������������ݣ���д���ļ� */
	int len = 0;

	while (1)
	{

		PackInfo pack_info; //����ȷ�ϰ�����

		if ((len = recvfrom(client_socket_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&server_addr, &server_addr_length)) > 0)
		{
			printf("len =%d\n", len);
			crc32tmp = crc32(crc, data.buf, sizeof(data));

			//crc32tmp=5;
			printf("-------------------------\n");
			printf("data.head.id=%ld\n", data.head.id);
			printf("id=%ld\n", id);

			if (data.head.id == id)
			{
				printf("crc32tmp=0x%x\n", crc32tmp);
				printf("data.head.crc32val=0x%x\n", data.head.crc32val);
				//printf("data.buf=%s\n",data.buf);

				//У��������ȷ
				if (data.head.crc32val == crc32tmp)
				{
					printf("rec data success\n");

					pack_info.id = data.head.id;
					pack_info.buf_size = data.head.buf_size;    //�ļ�����Ч�ֽڵĸ�������Ϊд���ļ�fwrite���ֽ���
					++id; //������ȷ��׼��������һ������

						  /* �������ݰ�ȷ����Ϣ */
					if (sendto(client_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
					/* д���ļ� */
					if (fwrite(data.buf, sizeof(char), data.head.buf_size, fp) < data.head.buf_size)
					{
						printf("File:\t%s Write Failed\n", file_name);
						break;
					}
				}
				else
				{
					pack_info.id = data.head.id;                //��������÷������ط�һ�� 
					pack_info.buf_size = data.head.buf_size;
					pack_info.errorflag = 1;

					printf("rec data error,need to send again\n");
					/* �ط����ݰ�ȷ����Ϣ */
					if (sendto(client_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
					{
						printf("Send confirm information failed!");
					}
				}

			}//if(data.head.id == id)  
			else if (data.head.id < id) /* ������ط��İ� */
			{
				pack_info.id = data.head.id;
				pack_info.buf_size = data.head.buf_size;

				pack_info.errorflag = 0;  //�������־����

				printf("data.head.id < id\n");
				/* �ط����ݰ�ȷ����Ϣ */
				if (sendto(client_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&server_addr, server_addr_length) < 0)
				{
					printf("Send confirm information failed!");
				}
			}//else if(data.head.id < id) /* ������ط��İ� */ 

		}
		else    //��������˳�
		{
			break;
		}


	}


	printf("Receive File:\t%s From Server IP Successful!\n", file_name);
	fclose(fp);
//	close(client_socket_fd);
	return 0;


}