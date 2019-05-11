#include<stdlib.h>
#include<stdio.h>
#include<winsock2.h>


#include"pkg_pro.h"

#pragma comment(lib,"ws2_32.lib")

sqlite3 *db;//sqlite3数据库初始化信息
SOCKET serverSocket;
int it_length = 0;//当前存入的ID数目
int last;//接受数据的长度
short int old_id_table[IDTABLE_SIZE];//原始ID表
short int new_id_table[IDTABLE_SIZE];//更改后的ID表

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

int main() {

	WSADATA WSAData;//windows socket初始化信息
	char receiveBuffer[BUFFER_SIZE];
	int rc;
	
	rc = sqlite3_open("data.db", &db);
 
	init_table(old_id_table, -1);//初始化原始ID表
	init_table(new_id_table, -1);//初始化更改后的ID表
	init_database(db, rc);//初始化数据库
	

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

	SOCKADDR_IN serverAddress; //服务器的地址等信息
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(53);
	serverAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{//服务器与本地地址绑定
		printf("Failed socket() %d \n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addr_Clt;

	int fromlen = sizeof(SOCKADDR);
	while (1)
	{
		last = recvfrom(serverSocket, receiveBuffer, BUFFER_SIZE, 0, (SOCKADDR*)&addr_Clt, &fromlen);
		if (last > 0)
		{      //判断接收到的数据是否为空
			receiveBuffer[last] = '\0';//给字符数组加一个'\0'，表示结束了。不然输出有乱码
			if (strcmp(receiveBuffer, "bye") == 0)
			{
				printf("客户端不跟我聊天了...");
				closesocket(serverSocket);
				return 0;
			}
			else
			{
                //创建header
				dns_header *header;
				header = (dns_header *)receiveBuffer;
				header->ID = ntohs(header->ID);
				header->FLAGS = ntohs(header->FLAGS);
				header->QDCOUNT = ntohs(header->QDCOUNT);
				header->ANCOUNT = ntohs(header->ANCOUNT);
				header->NSCOUNT = ntohs(header->NSCOUNT);
				header->ARCOUNT = ntohs(header->ARCOUNT);

				//printf("\n-----ID:%d FLAGS:%d qcount:%d------\n%d\n", header->ID, header->FLAGS, header->QDCOUNT,last);


				if ((header->FLAGS & 0x8000) == 0x8000)//为响应包
				{
					//printf("\n\n-------Response Package-------\n");
					resp_pro(header, receiveBuffer);
				}
				else//请求包
				{
					//printf("\n\n-------Query Package-------\n");
					query_pro(header, receiveBuffer, addr_Clt);//请求处理
				}


			}
		}
	}
	closesocket(serverSocket);
    WSACleanup();
	sqlite3_close(db);
	return 0;
}
