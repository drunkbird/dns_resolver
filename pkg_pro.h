#ifndef PKG_PRO_H__
#define PKG_PRO_H__

#include<winsock2.h>
#include<string>
#include<iostream>
#include<map>
#include<sstream>
#include<time.h>

#include"sqlite3.h"

#define IDTABLE_SIZE 256

#define BUFFER_SIZE 1024
#define QNAME_MAX_LENTH 256
#define SUPERIOR_SERVER_ADDRESS "10.3.9.5"
#define SQL_MAX 4096
#define RESO_MAX 1024 //��Դ����¼��������1024
#define DOMAIN_LEN_MAX 256//���������


using namespace std;


//dns��ͷ�ṹ
/*-------------------------------------*/
/*                  ID                 */
/*-------------------------------------*/
/*                 FLAGS               */
/*-------------------------------------*/
/*                QDCOUNT              */
/*-------------------------------------*/
/*                ANCOUNT              */
/*-------------------------------------*/
/*                NSCOUNT              */
/*-------------------------------------*/
/*                ARCOUNT              */
/*-------------------------------------*/


typedef struct
{
	unsigned short ID;
	unsigned short FLAGS;
	unsigned short QDCOUNT;
	unsigned short ANCOUNT;
	unsigned short NSCOUNT;
	unsigned short ARCOUNT;
} dns_header;

//dns��Դ��¼�ṹ
/*-------------------------------------*/
/*                NAME                 */
/*-------------------------------------*/
/*                TYPE                 */
/*-------------------------------------*/
/*                CLASS                */
/*-------------------------------------*/
/*                TTL                  */
/*                                     */
/*-------------------------------------*/
/*               RDLENGTH              */
/*-------------------------------------*/
/*                RDATA                */
/*-------------------------------------*/


//dns��Դ��¼�ṹ
typedef struct 
{
	char *NAME;//��Դ��¼ƥ�������
	unsigned short TYPE;//��Դ����
	unsigned short CLASS;//�涨RDATA������
	unsigned long TTL;//����ʱ��
	unsigned short DATALENGTH;//RDATA�ֶγ���
	char *RDATA;//��Դ
	unsigned short PREFERENCE;//MX��¼���ȼ�
} resRecord;

extern int it_length;//��ǰ�����ID��Ŀ
extern int last;//�������ݵĳ���
extern long timestamp[IDTABLE_SIZE];//������ϼ����������ʱ��
extern SOCKADDR_IN client_ip[IDTABLE_SIZE];//��ſͻ�����ip��ַ�����Է��𸴰��Լ���������
extern short int old_id_table[IDTABLE_SIZE];//ԭʼID��
extern short int new_id_table[IDTABLE_SIZE];//���ĺ��ID����ID��С�����˳��洢
extern sqlite3 *db;//sqlite3���ݿ��ʼ����Ϣ
extern map<string, unsigned short> *mapDomainName;

extern SOCKET serverSocket;

void init_table(short int t[], short int q);//�����ʼ��
void query_pro(dns_header *header, char *receiveBuffer, SOCKADDR_IN cli_ip);//���������
void query_for_superior_server(char *receiveBuffer, dns_header *header, SOCKADDR_IN cli_ip);//ת������һ������������
void resp_pro(dns_header *header, char *receiveBuffer);//��Ӧ������
void connect_string(char *a, char *b);
void connect_const_string(char *a, const char *b);

int query_A_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records);
int query_A_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, const char *Address, int addLength);
void insert_A_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength,const char *Address, int *length);

void insert_CNAME_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, char *CNAME, int *length);
int query_CNAME_record(sqlite3 *db, char *zErrMsg, char *Alias, int nameLength, resRecord *record);//��ѯ�ض�Ȩ������¼
int query_CNAME_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *CNAME, int CNLength);

void connect_string(char *a, const char *b, int aLength, int bLength);//�����ַ���
void connect_string(char *a, char *b, int aLength, int bLength);//�����ַ���

string translate_IP(unsigned char* ip);
void insert_IP(char *ip, char *sendBuf, int *bytePos);//��ip���뷢�ͻ�����

int do_name_reso(int clength, int addlength, int c_byte, char doname[], char *receiveBuffer);//��������

void a_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos);//a���ͼ�¼����
void cn_records_pro(resRecord record, char *sendBuf, int *bytePos);//cname���ͼ�¼����
void cn_records_pro(resRecord record, char *sendBuf, int *bytePos, int len);//cn_records_pro���أ�ִֻ��һ��
void domain_pro(char* name, char *sendBuf, int *bytePos);//��������
void domainStore(char *domain, int len, int iniBytePos, string res);//���������ִ����ֵ���

void insert_NS_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, char *NS, int *length);//����NS���ͼ�¼
void insert_MX_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, int Preference, char *MX, int *length);//����MX���ͼ�¼

int query_MX_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records);
int query_MX_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *mName, int mNameLen);
int query_NS_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *NAME_SERVER, int nameServerLen);//��ѯNS��¼
int query_NS_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records);//��ѯNS��¼������

void ns_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos);
void mx_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos);
#endif // PKG_PRO_H__

