// UDP-SendFile.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include<sys/types.h> 
#include<stdio.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<stdarg.h> 
#include<string> 
#include <WinSock2.h>


#define SERVER_PORT 8000 
#define BUFFER_SIZE 500             //�����ļ�udp��������С
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
struct SendPack
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
static unsigned int crc32(unsigned int crc, char *buffer, unsigned int size)
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
	if (LOBYTE(ws.wVersion) != 2|| HIBYTE(ws.wVersion) != 2)
	{
		WSACleanup();
		return 1; 
	}

	/* ����id */
	long int send_id = 0;

	/* ����id */
	int receive_id = 0;

	/* ����UDP�׽ӿ� */
	struct sockaddr_in server_addr;
//	bzero(&server_addr, sizeof(server_addr));
	memset(&server_addr, 0x00, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(SERVER_PORT);

	/* ����socket */
	SOCKET server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_socket_fd == -1)
	{
		
		printf("Create Socket Failed:");

		exit(1);
	}

	/* ���׽ӿ� */
	if (-1 == (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))))
	{
		printf("Server Bind Failed:");
		exit(1);
	}


	//crc32
	init_crc_table();

	/* ���ݴ��� */
	while (1)
	{
		/* ����һ����ַ�����ڲ���ͻ��˵�ַ */
		struct sockaddr_in client_addr;
		int client_addr_length = sizeof(client_addr);

		/* �������� */
		char buffer[BUFFER_SIZE];
		//bzero(buffer, BUFFER_SIZE);
		memset(buffer, 0x00, BUFFER_SIZE);
		if (recvfrom(server_socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_length) == -1)
		{
			printf("Receive Data Failed:");
			exit(1);
		}

		/* ��buffer�п�����file_name */
		char file_name[FILE_NAME_MAX_SIZE + 1];
		//bzero(file_name, FILE_NAME_MAX_SIZE + 1);
		memset(file_name, 0x00, FILE_NAME_MAX_SIZE + 1);
		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE ? FILE_NAME_MAX_SIZE : strlen(buffer));
		printf("%s\n", file_name);

		/* ���ļ� */
		FILE *fp = fopen(file_name, "r");
		if (NULL == fp)
		{
			printf("File:%s Not Found.\n", file_name);
		}
		else
		{
			int len = 0;
			/* ÿ��ȡһ�����ݣ��㽫�䷢���ͻ��� */
			while (1)
			{
				PackInfo pack_info;
			//	bzero((char *)&data, sizeof(data));  //ljh socket���ͻ���������
				memset((char *)&data, 0x00, sizeof(data));
				printf("receive_id=%d\n", receive_id);
				printf("send_id=%ld\n", send_id);

				if (receive_id == send_id)
				{
					++send_id;
					if ((len = fread(data.buf, sizeof(char), BUFFER_SIZE, fp)) > 0)
					{
						data.head.id = send_id; /* ����id�Ž���ͷ,���ڱ��˳�� */
						data.head.buf_size = len; /* ��¼���ݳ��� */
						data.head.crc32val = crc32(crc, data.buf, sizeof(data));
						printf("len =%d\n", len);
						printf("data.head.crc32val=0x%x\n", data.head.crc32val);
						//printf("data.buf=%s\n",data.buf);
					resend:
						if (sendto(server_socket_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
						{
							printf("Send File Failed:");
							break;
						}
						/* ����ȷ����Ϣ */
						recvfrom(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
						receive_id = pack_info.id;
						//���ȷ�ϰ���ʾ���ݴ���
						if (pack_info.errorflag == 1)
						{
							pack_info.errorflag = 0;
							goto  resend;
						}
						//usleep(50000);   
					}
					else
					{
						break;
					}
				}
				else
				{
					/* ������յ�id�ͷ��͵�id����ͬ,���·��� */
					if (sendto(server_socket_fd, (char*)&data, sizeof(data), 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
					{
						printf("Send File Failed:");
						break;
					}
					printf("repeat send\n");
					/* ����ȷ����Ϣ */
					recvfrom(server_socket_fd, (char*)&pack_info, sizeof(pack_info), 0, (struct sockaddr*)&client_addr, &client_addr_length);
					receive_id = pack_info.id;

					//usleep(50000); 
				}
			}

			//���ͽ����� 0�ֽ�Ŀ�ĸ��߿ͻ��˷������
			if (sendto(server_socket_fd, (char*)&data, 0, 0, (struct sockaddr*)&client_addr, client_addr_length) < 0)
			{
				printf("Send 0 char  Failed:");
				break;
			}
			printf("sever send file end 0 char\n");
			/* �ر��ļ� */
			fclose(fp);
			printf("File:%s Transfer Successful!\n", file_name);

			//����id��׼��������һ���ļ�
			/* ����id */
			send_id = 0;
			/* ����id */
			receive_id = 0;
		}
	}
	//CloseHandle(server_socket_fd)
	//close(server_socket_fd);
	return 0;
}