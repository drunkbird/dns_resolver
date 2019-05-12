#include<stdlib.h>
#include<stdio.h>
#include<winsock2.h>


#include"pkg_pro.h"

#pragma comment(lib,"ws2_32.lib")

sqlite3 *db;//sqlite3���ݿ��ʼ����Ϣ
SOCKET serverSocket;
int it_length = 0;//��ǰ�����ID��Ŀ
int last;//�������ݵĳ���
short int old_id_table[IDTABLE_SIZE];//ԭʼID��
short int new_id_table[IDTABLE_SIZE];//���ĺ��ID��

int dFlag = 0;
int ddFlag = 0;
char SUPERIOR_SERVER_ADDRESS[15] = "10.3.9.5";
char filePath[SQL_MAX] = "data.db";

void init_database(sqlite3 *db, int rc)
{
	char *zErrMsg = 0;
	//char *sql;
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
		fprintf(stderr, "Opened database successfully\n");
	}
}

int main(int argc, char *argv[]) 
{
	int nsFlag = 0, fileFlag = 0;//�ֱ��ʶ����������Ƿ������ַ�������ָ��, �����ļ�·����ָ��
	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-d"))
		{
			if (!dFlag && !ddFlag && !nsFlag && !fileFlag)
			{
				dFlag = 1;
			}
			else
			{
				printf("ERROR: debugging information parameter\n");
				exit(0);
			}
		}
		else if (!strcmp(argv[i], "-dd"))
		{
			if (!dFlag && !ddFlag && !nsFlag && !fileFlag)
			{
				ddFlag = 1;
			}
			else
			{
				printf("ERROR: debugging information parameter\n");
				exit(0);
			}
		}
		else if (argv[i][0] >= '0' && argv[i][0] <= '9')
		{
			if (!nsFlag && !fileFlag)
			{
				nsFlag = 1;
				for (int i = 0; i < 15; i++)
				{
					SUPERIOR_SERVER_ADDRESS[i] = 0;
				}
				connect_string(SUPERIOR_SERVER_ADDRESS, argv[i], 0, strlen(argv[i]));
			}
			else
			{
				printf("ERROR: debugging information parameter\n");
				exit(0);
			}
		}
		else
		{
			if (!fileFlag)
			{
				fileFlag = 1;
				for (int i = 0; i < SQL_MAX; i++)
				{
					filePath[i] = 0;
				}
				connect_string(filePath, argv[i], 0, strlen(argv[i]));
			}
			else
			{
				printf("ERROR: debugging information parameter\n");
				exit(0);
			}
		}

	}
	

	WSADATA WSAData;//windows socket��ʼ����Ϣ
	char receiveBuffer[BUFFER_SIZE];
	int rc;
	
	rc = sqlite3_open(filePath, &db);
 
	init_table(old_id_table, -1);//��ʼ��ԭʼID��
	init_table(new_id_table, -1);//��ʼ�����ĺ��ID��
	init_database(db, rc);//��ʼ�����ݿ�
	

	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		printf("fail to initialize\n");
		exit(0);
	}

	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("fail to create socket\n");
		exit(0);
	}

	SOCKADDR_IN serverAddress; //�������ĵ�ַ����Ϣ
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(53);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{//�������뱾�ص�ַ��
		printf("Failed socket() %d \n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addr_Clt;

	int fromlen = sizeof(SOCKADDR);

	char *zErrMsg = 0;
	thread t1(delete_expired_data, db, zErrMsg);//Ϊdelete_expired_data(sqlite3 *db, char *zErrMsg)����һ���߳�
	t1.detach();

	while (1)
	{
		last = recvfrom(serverSocket, receiveBuffer, BUFFER_SIZE, 0, (SOCKADDR*)&addr_Clt, &fromlen);
		if (last > 0)
		{      //�жϽ��յ��������Ƿ�Ϊ��
			receiveBuffer[last] = '\0';//���ַ������һ��'\0'����ʾ�����ˡ���Ȼ���������
			if (strcmp(receiveBuffer, "bye") == 0)
			{
				printf("�ͻ��˲�����������...");
				closesocket(serverSocket);
				return 0;
			}
			else
			{
                //����header
				dns_header *header;
				header = (dns_header *)receiveBuffer;
				header->ID = ntohs(header->ID);
				header->FLAGS = ntohs(header->FLAGS);
				header->QDCOUNT = ntohs(header->QDCOUNT);
				header->ANCOUNT = ntohs(header->ANCOUNT);
				header->NSCOUNT = ntohs(header->NSCOUNT);
				header->ARCOUNT = ntohs(header->ARCOUNT);

				//printf("\n-----ID:%d FLAGS:%d qcount:%d------\n%d\n", header->ID, header->FLAGS, header->QDCOUNT,last);


				if ((header->FLAGS & 0x8000) == 0x8000)//Ϊ��Ӧ��
				{
					/*if (dFlag || ddFlag)
					{
						printf("\n\n-------Response Package-------\n");
					}*/
					resp_pro(header, receiveBuffer);
				}
				else//�����
				{
					//if (dFlag || ddFlag)
					//{
					//	//printf("\n\n-------Query Package-------\n");
					//}
					query_pro(header, receiveBuffer, addr_Clt);//������
				}


			}
		}
	}
	closesocket(serverSocket);
    WSACleanup();
	sqlite3_close(db);
	return 0;
}
