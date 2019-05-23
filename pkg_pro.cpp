#include<stdlib.h>
#include<stdio.h>
#include<mutex>

#include"pkg_pro.h"

long timestamp[IDTABLE_SIZE];//������ϼ����������ʱ��
SOCKADDR_IN client_ip[IDTABLE_SIZE];//��ſͻ�����ip��ַ�����Է��𸴰��Լ���������
stringstream sstream;//ת����ʽ��
map<string, unsigned short> *mapDomainName;

std::mutex mt;//������

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {//��ӡ������Ϣ���
	int i;
	for (i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int str_len(char *str)//��ĳ�ַ�������
{
	int i = 0;
	while (str[i])
	{
		i++;
	}
	return i;
}

string translate_IP(unsigned char* ip)//����unsigned char���ʹ洢��ip��ַת�����ַ���
{
	string result = "";
	for (int i = 0; i < 4; i++)
	{
		result = result + to_string(ip[i]) + ".";
	}
	return result.substr(0, result.length() - 1);
}

void insert_IP(char *ip, char *sendBuf, int *bytePos)//��ip���뷢�ͻ�����
{
	int ipLen = str_len(ip);
	string tmp = "";
	for (int i = 0; i <= ipLen; i++)
	{
		if (i != ipLen && ip[i] != '.')//δ����β��δ����'.'������string
		{
			char s[2] = { ip[i], 0 };
			string c = s;
			tmp = tmp + c;
		}
		else//ת�����ֽڴ��뷢�ͻ�����
		{
			sstream.clear();//���sstream������
			unsigned short singleIP;
			sstream << tmp;
			sstream >> singleIP;
			sstream.str("");
			sendBuf[*bytePos] = (char)singleIP;
			*bytePos = *bytePos + 1;
			tmp = "";
		}
	}
}

void init_table(short int t[], short int q)
{
	for (int i = 0; i < IDTABLE_SIZE; i++)
	{
		t[i] = q;
	}
}

void domainStore(char *domain, int len, int iniBytePos, string res)//����-�ֽ�λ�ô洢�Ա�ʵ��ѹ������
{	
	if (len <= 0) return;
	string tmp =  (res == "") ? "" : ('.' + res);//Ϊ�˴洢��������
	int index = len - 1;
	while (index >= -1)
	{
		if (domain[index] != '.' && index != -1)//Ϊ�����ַ�
		{
			char s[2] = { domain[index], 0 };
			string c = s;
			tmp = c + tmp;
		}
		else
		{
			unsigned short namePos = (unsigned short)(iniBytePos + index + 1);
			mapDomainName->insert(pair<string, unsigned short>(tmp, namePos));
			tmp = '.' + tmp;
		}
		index--;
	}
}

unsigned short domain_pro(char* name, char *sendBuf, int *bytePos)
{
	int j = str_len(name) - 1;
	int nameEndPos = j + 1;//��ѯname�����ֹ��λ��
	unsigned short ptrPos, dataLen = 0;//ָ�붨λ��ֵ
	string tmp = "", res = "";
	while (j >= -1)//����ѹ����λ
	{
		if (name[j] != '.' && j != -1)
		{
			char s[2] = { name[j], 0 };
			string c = s;
			tmp = c + tmp;
		}
		else//����.���жϸ������Ƿ��Ѿ�����
		{
			map<string, unsigned short>::iterator iter;
			iter = mapDomainName->find(tmp);
			if (iter != mapDomainName->end())
			{
				nameEndPos = j;
				ptrPos = iter->second;
				res = iter->first;
			}
			else
				break;
			tmp = '.' + tmp;
		}
		j--;
	}
	int i = 0, lastPos = 0, iniBytePos = *bytePos;
	while (i <= (nameEndPos))//���ȫ������ָ���������Ҫ�����Ĳ���
	{
		if (name[i] == '.' or name[i] == '\0')
		{
			sendBuf[*bytePos] = i - lastPos;
			dataLen = dataLen + 1;//dataLenֵ����
			for (int j = 0; j < i - lastPos; j++)//���������뷢�ͻ�����
			{
				dataLen = dataLen + 1;//dataLenֵ����
				*bytePos = *bytePos + 1;
				sendBuf[*bytePos] = name[j + lastPos];
			}
			*bytePos = *bytePos + 1;
			lastPos = i + 1;
		}
		i++;
	}
	i--;//i��λ��'\0'
	//��ָ����뷢�ͻ�����
	if (res != "")//ʹ����ָ��
	{
		dataLen = dataLen + 2;
		sendBuf[*bytePos] = (char)192;
		*bytePos = *bytePos + 1;

		sendBuf[*bytePos] = (char)ptrPos;
		*bytePos = *bytePos + 1;	
	}
	else
	{
		sendBuf[*bytePos] = 0;
		*bytePos = *bytePos + 1;
	}
	

	//����-�ֽ�λ�ô洢
	domainStore(name, i, iniBytePos, res);
	return dataLen;
}

void a_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos)
{
	for (int i = 0; i < len; i++)
	{
		domain_pro(records[i].NAME, sendBuf, bytePos);//��������
		sendBuf[*bytePos + 1] = (char)(records[i].TYPE);
		sendBuf[*bytePos + 0] = (records[i].TYPE) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 1] = (char)(records[i].CLASS);
		sendBuf[*bytePos + 0] = (records[i].CLASS) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 3] = (char)(records[i].TTL);
		sendBuf[*bytePos + 2] = (char)((records[i].TTL) >> 8);
		sendBuf[*bytePos + 1] = (char)((records[i].TTL) >> 16);
		sendBuf[*bytePos + 0] = (records[i].TTL) >> 24;
		*bytePos = *bytePos + 4;
		sendBuf[*bytePos + 1] = (char)records[i].DATALENGTH;
		sendBuf[*bytePos + 0] = records[i].DATALENGTH >> 8;
		*bytePos = *bytePos + 2;
		insert_IP(records[i].RDATA, sendBuf, bytePos);
	}
}

void cn_records_pro(resRecord record, char *sendBuf, int *bytePos)
{
	resRecord newRecord = record;
	char *cname = record.NAME;
	char *zErrMsg = 0;
	while (query_CNAME_record(db, zErrMsg, cname, str_len(cname), &newRecord))
	{
		domain_pro(newRecord.NAME, sendBuf, bytePos);//��������
		sendBuf[*bytePos + 1] = (char)(newRecord.TYPE);
		sendBuf[*bytePos + 0] = (newRecord.TYPE) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 1] = (char)(newRecord.CLASS);
		sendBuf[*bytePos + 0] = (newRecord.CLASS) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 3] = (char)(newRecord.TTL);
		sendBuf[*bytePos + 2] = (char)((newRecord.TTL) >> 8);
		sendBuf[*bytePos + 1] = (char)((newRecord.TTL) >> 16);
		sendBuf[*bytePos + 0] = (newRecord.TTL) >> 24;
		*bytePos = *bytePos + 4;
		int dataLenPos = *bytePos;
		*bytePos = *bytePos + 2;
		unsigned short dataLen = domain_pro(newRecord.RDATA, sendBuf, bytePos);//���data�ֶ���ռ�ֽ���
		sendBuf[dataLenPos + 1] = (char)dataLen;
		sendBuf[dataLenPos + 0] = dataLen >> 8;
		cname = newRecord.RDATA;//������ѯ��һ��CN
	}
}

void cn_records_pro(resRecord record, char *sendBuf, int *bytePos, int len)//cn_records_pro���أ�ִֻ��һ��
{
	resRecord newRecord = record;
	char *cname = record.NAME;
	char *zErrMsg = 0;
	domain_pro(newRecord.NAME, sendBuf, bytePos);//��������
	sendBuf[*bytePos + 1] = (char)(newRecord.TYPE);
	sendBuf[*bytePos + 0] = (newRecord.TYPE) >> 8;
	*bytePos = *bytePos + 2;
	sendBuf[*bytePos + 1] = (char)(newRecord.CLASS);
	sendBuf[*bytePos + 0] = (newRecord.CLASS) >> 8;
	*bytePos = *bytePos + 2;
	sendBuf[*bytePos + 3] = (char)(newRecord.TTL);
	sendBuf[*bytePos + 2] = (char)((newRecord.TTL) >> 8);
	sendBuf[*bytePos + 1] = (char)((newRecord.TTL) >> 16);
	sendBuf[*bytePos + 0] = (newRecord.TTL) >> 24;
	*bytePos = *bytePos + 4;
	int dataLenPos = *bytePos;
	*bytePos = *bytePos + 2;
	unsigned short dataLen = domain_pro(newRecord.RDATA, sendBuf, bytePos);//���data�ֶ���ռ�ֽ���
	sendBuf[dataLenPos + 1] = (char)dataLen;
	sendBuf[dataLenPos + 0] = dataLen >> 8;
	
}

void ns_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos)
{
	for (int i = 0; i < len; i++)
	{
		domain_pro(records[i].NAME, sendBuf, bytePos);//��������
		sendBuf[*bytePos + 1] = (char)(records[i].TYPE);
		sendBuf[*bytePos + 0] = (records[i].TYPE) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 1] = (char)(records[i].CLASS);
		sendBuf[*bytePos + 0] = (records[i].CLASS) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 3] = (char)(records[i].TTL);
		sendBuf[*bytePos + 2] = (char)((records[i].TTL) >> 8);
		sendBuf[*bytePos + 1] = (char)((records[i].TTL) >> 16);
		sendBuf[*bytePos + 0] = (records[i].TTL) >> 24;
		*bytePos = *bytePos + 4;
		int dataLenPos = *bytePos;
		*bytePos = *bytePos + 2;
		unsigned short dataLen = domain_pro(records[i].RDATA, sendBuf, bytePos);//���data�ֶ���ռ�ֽ���
		sendBuf[dataLenPos + 1] = (char)dataLen;
		sendBuf[dataLenPos + 0] = dataLen >> 8;	
	}
}

void mx_records_pro(resRecord *records, int len, char *sendBuf, int *bytePos)
{
	for (int i = 0; i < len; i++)
	{
		domain_pro(records[i].NAME, sendBuf, bytePos);//��������
		sendBuf[*bytePos + 1] = (char)(records[i].TYPE);
		sendBuf[*bytePos + 0] = (records[i].TYPE) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 1] = (char)(records[i].CLASS);
		sendBuf[*bytePos + 0] = (records[i].CLASS) >> 8;
		*bytePos = *bytePos + 2;
		sendBuf[*bytePos + 3] = (char)(records[i].TTL);
		sendBuf[*bytePos + 2] = (char)((records[i].TTL) >> 8);
		sendBuf[*bytePos + 1] = (char)((records[i].TTL) >> 16);
		sendBuf[*bytePos + 0] = (records[i].TTL) >> 24;
		*bytePos = *bytePos + 4;
		int dataLenPos = *bytePos;
		*bytePos = *bytePos + 4;
		unsigned short dataLen = domain_pro(records[i].RDATA, sendBuf, bytePos) + 2;////���data�ֶ���ռ�ֽ�����+2�����ڰ���preference�ֶ����ֽ�
		sendBuf[dataLenPos + 1] = (char)dataLen;
		sendBuf[dataLenPos + 0] = dataLen >> 8;
		sendBuf[dataLenPos + 3] = (char)records[i].PREFERENCE;
		sendBuf[dataLenPos + 2] = records[i].PREFERENCE >> 8;
	}
}

void query_for_superior_server(char *receiveBuffer, dns_header *header, SOCKADDR_IN cli_ip)
{
	//ת��DNS���ݰ�ͷID,��ͻ���ip��ַ
	long current_time = GetTickCount();//��¼�½��յ�����ʱ��
	int k = 0;//�ж��Ƿ�����ͬID
	short int hid = header->ID;//ȡID�ֶ�
	short int nhid;//���ĺ��ID
	for (int i = 0; i < it_length; i++)
	{
		if (hid == new_id_table[i]) 
		{//������ͬID
			k = 1;
			break;
		}
	}
	int i = 0;
	if (k == 0) 
	{//û����ͬ��ID
		nhid = hid;//���ø���ID
		for (i = 0; i < it_length; i++)
		{
			if (nhid < new_id_table[i]) {//��ID������ĺ�ı�
				for (int j = it_length; j > i; j--)
				{//���ڴ�ID�İ���Ϣ����
					new_id_table[j] = new_id_table[j - 1];
					old_id_table[j] = old_id_table[j - 1];
					client_ip[j] = client_ip[j - 1];
					timestamp[j] = timestamp[j - 1];
				}
				//���µ����ϼ���������洢��Ϣ
				old_id_table[i] = hid;
				new_id_table[i] = nhid;
				client_ip[i] = cli_ip;
				it_length++;
				break;
			}
		}
		if (i == it_length)
		{//���±�������ID��С�ڴ�ID
			old_id_table[i] = hid;
			new_id_table[i] = nhid;
			client_ip[i] = cli_ip;
			it_length++;
		}
	}
	else
	{//����ͬ��id
		nhid = 0;//��0��ʼ����ID
		int i = 0;
		for (int i = 0; i < it_length; i++)
		{//��һ��δ��ʹ�õ�ID��ɱ��滻��ID
			if ((nhid == new_id_table[i]) && (current_time - timestamp[i] <= 50000))  nhid++;//�����ID�ѱ�ʹ��
			else {//�����IDδ��ʹ�û�ʱʱ������ɱ��滻������������ģ���ֱ���ڴ˲���
				for (int j = it_length; j > i; j--)
				{//����
					new_id_table[j] = new_id_table[j - 1];
					old_id_table[j] = old_id_table[j - 1];
					client_ip[j] = client_ip[j - 1];
					timestamp[j] = timestamp[j - 1];
				}
				//�洢��ǰ����Ϣ
				old_id_table[i] = hid;
				new_id_table[i] = nhid;
				client_ip[i] = cli_ip;
				it_length++;
				break;
			}
		}
		if (i == it_length)
		{//���±�������ID��С�ڴ�ID
			old_id_table[i] = hid;
			new_id_table[it_length] = nhid;
			client_ip[i] = cli_ip;
			it_length++;
		}
	}
	header->ID = nhid;//����ID�ֶθ�����ͷ
	header->ID = htons(header->ID);//ת���ֽ���
	
	

	SOCKADDR_IN to_address;
	int addr_len = sizeof(SOCKADDR_IN);
	to_address.sin_family = AF_INET;
	to_address.sin_port = htons(53);
	to_address.sin_addr.S_un.S_addr = inet_addr(SUPERIOR_SERVER_ADDRESS);
	if (sendto(serverSocket, receiveBuffer, last, 0, (const struct sockaddr *)&to_address, addr_len))
	{
		timestamp[i] = GetTickCount();//��¼��������ʱ��
		//printf("ת������һ�������������ɹ���\n");
	}
	else
	{
		//printf("ת������һ������������ʧ�ܣ�\n");
		for (int j = i; j < IDTABLE_SIZE; i++)
		{//���ĺ����Ϣɾ��
			new_id_table[j] = new_id_table[j + 1];
			old_id_table[j] = old_id_table[j + 1];
			client_ip[j] = client_ip[j + 1];
			timestamp[j] = timestamp[j + 1];
		}
		it_length--;
	}
}

void query_pro(dns_header *header, char *receiveBuffer, SOCKADDR_IN cli_ip)
{
	char *question_sec = receiveBuffer + sizeof(dns_header);
	char QNAME[QNAME_MAX_LENTH + 1];
	int index = 0, now_pos = 0;
	while (question_sec[index] != 0x00)//��ȡ����
	{
		int temp = question_sec[index], i;
		for (i = 0; i < temp; i++)
		{
			QNAME[i + now_pos] = question_sec[index + i + 1];
		}
		index += temp + 1;
		now_pos += temp;
		if (question_sec[index] != 0x00)
			QNAME[now_pos++] = '.';
	}
	QNAME[now_pos] = '\0';

	if (dFlag)
	{
		printf("time = %lld\nID = %u\nclientIP = %s\nrequestedDomainName = %s\n\n\n", time(NULL), header->ID, inet_ntoa(cli_ip.sin_addr), QNAME);
	}
	if (ddFlag)
	{
		printf("    HEADER:\n");
		printf("        opcode = ");
		if ((header->FLAGS & 0x7800) == 0x0000)
		{
			printf("Query, ");
		}
		else if ((header->FLAGS & 0x7800) == 0x0800)
		{
			printf("IQuery, ");
		}
		else if ((header->FLAGS & 0x7800) == 0x1000)
		{
			printf("Status, ");
		}
		else if ((header->FLAGS & 0x7800) == 0x2000)
		{
			printf("Notify, ");
		}
		else if ((header->FLAGS & 0x7800) == 0x2800)
		{
			printf("Update, ");
		}
		else
		{
			printf("Unassigned, ");
		}
		printf("id = %u, ", header->ID);
		printf("rcode = ");
		if ((header->FLAGS & 0x000F) == 0x0000)
		{
			printf("NoError\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0001)
		{
			printf("FormErr\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0002)
		{
			printf("ServFail\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0003)
		{
			printf("NXDomain\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0004)
		{
			printf("NotImp\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0005)
		{
			printf("Refused\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0006)
		{
			printf("YXDomain\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0007)
		{
			printf("YXRRSet\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0008)
		{
			printf("NXRRSet\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x0009)
		{
			printf("NotAuth\n");
		}
		else if ((header->FLAGS & 0x000F) == 0x000A)
		{
			printf("NotZone\n");
		}
		else
		{
			printf("Unassigned\n");
		}
		printf("        questions = %u, answers = %u, authority records = %u, additional = %u\n\n", header->QDCOUNT, header->ANCOUNT, header->NSCOUNT, header->ARCOUNT);
		printf("    QUESTIONS:\n");
		printf("        QName = %s\n", QNAME);
		

	}

	//���յ�header���ֽ����Ϊ�����ֽ���
	
	header->FLAGS = htons(header->FLAGS);
	header->QDCOUNT = htons(header->QDCOUNT);
	header->ANCOUNT = htons(header->ANCOUNT);
	header->NSCOUNT = htons(header->NSCOUNT);
	header->ARCOUNT = htons(header->ARCOUNT);

	/*�жϱ������ݿ��Ƿ���и�������¼*/
	char doName[QNAME_MAX_LENTH];//��ѯ�������ʼ���ַ��׺��
	int length = 0;//�������ʼ���ַ��׺��ռ�õ��ֽ���
	int c_byte = sizeof(dns_header);//��ǰ��ֵ����ֽ�λ��
	unsigned short *type;//��ѯ����

	length = do_name_reso(0, 0, c_byte, doName, receiveBuffer);//�����������ʼ���ַ��׺��
	c_byte += length;
	type = (unsigned short*)(receiveBuffer + c_byte);
	c_byte += 2;

	int tp = ntohs(*type);
	if (ddFlag)
	{
		printf("        QType = %d\n", tp);
		printf("        QClass = %d\n", ntohs(*(unsigned short*)(receiveBuffer + c_byte)));
	}
	//printf("�����ѯ����: %d\n",tp);
	char *zErrMsg = 0;

	char *sendBuf = new char[BUFFER_SIZE];//���ͻ�����
	int bytePos = last;//���ͻ�������ǰ������ݵ�λ��
	
	delete mapDomainName;//�����¡�����-�ֽ�λ�á��ֵ�֮ǰ����֮ǰ�ֵ�ɾ��
	mapDomainName = new map<string, unsigned short>;//��������ѹ��������ֵ�

	if(tp == 1)//ΪA���Ͳ�ѯ����
	{	
		if (query_undesirable_web_record(db, zErrMsg, doName, str_len(doName)))//��վ������
		{
			header->ID = htons(header->ID);//ת���������ֽ���
			memcpy(sendBuf, receiveBuffer, last);//����N���ֽڵ����ͻ�����
			unsigned short Flags = 0x8183;//��վ������
			unsigned short Questions = (unsigned short)1;
			unsigned short Answer = (unsigned short)0;

			*(sendBuf + 3) = (char)Flags;
			*(sendBuf + 2) = Flags >> 8;
			*(sendBuf + 5) = (char)Questions;
			*(sendBuf + 4) = Questions >> 8;
			*(sendBuf + 7) = (char)Answer;
			*(sendBuf + 6) = Answer >> 8;
			//printf("----------\n");
			sendto(serverSocket, sendBuf, bytePos, 0, (SOCKADDR*)&cli_ip, sizeof(SOCKADDR));
		}
		else
		{
			resRecord cnameRecord[RESO_MAX];//�洢cname���ͼ�¼
			int queryCNameRes = query_CNAME_record(db, zErrMsg, doName, str_len(doName), cnameRecord, 1);
			resRecord aRecord[RESO_MAX];//�洢A���ͼ�¼
			int queryAResult = query_A_record(db, zErrMsg, doName, str_len(doName), aRecord);
			if (queryAResult)//�������ݿ��л���,���ͻ��˷���response��
			{
				header->ID = htons(header->ID);
				memcpy(sendBuf, receiveBuffer, last);//����N���ֽڵ����ͻ�����

				//��QNAME�����ֵ�
				domainStore(QNAME, str_len(QNAME), 12, "");//12ΪQNAME��ʼ�ֽ���
				//printf("%s/*/*/*/%d*/*/*/\n", QNAME, str_len(QNAME));

				unsigned short Flags = 0x8180;
				unsigned short Questions = (unsigned short)1;
				unsigned short Answer = (unsigned short)(queryCNameRes + queryAResult);
				//printf("/*/*/*/%c*/*answer/*/\n", Answer);
				//printf("/*/*/*/%u*/*/answer*/\n", Answer);
				*(sendBuf + 3) = (char)Flags;
				*(sendBuf + 2) = Flags >> 8;
				*(sendBuf + 5) = (char)Questions;
				*(sendBuf + 4) = Questions >> 8;
				*(sendBuf + 7) = (char)Answer;
				*(sendBuf + 6) = Answer >> 8;
				//printf("/*/*/*/%d/*%d/*/\n", sendBuf[6], sendBuf[7]);

				if (queryCNameRes)//�������CN���ͼ�¼
				{
					cn_records_pro(*cnameRecord, sendBuf, &bytePos);//cname��¼�����dns����������ʽ�����뷢�ͻ�����
				}
				a_records_pro(aRecord, queryAResult, sendBuf, &bytePos);

				printf("���ش���%s������A���ͼ�¼!\n\n", doName);
				//ת�����ͻ���
				sendto(serverSocket, sendBuf, bytePos, 0, (SOCKADDR*)&cli_ip, sizeof(SOCKADDR));
			}
			else
			{
				query_for_superior_server(receiveBuffer, header, cli_ip);//���һ���������������Ͳ�ѯ
			}
			if (!cnameRecord->NAME)
				delete cnameRecord->NAME;
			if (!cnameRecord->RDATA)
				delete cnameRecord->RDATA;
			if (!cnameRecord)
				delete cnameRecord;
		}
	}
	else if (tp == 5)//CNAME����
	{
		resRecord *cnameRecord = new resRecord;//�洢cname���ͼ�¼
		//cnameRecord ;
		int queryCNameRes = query_CNAME_record(db, zErrMsg, doName, str_len(doName), cnameRecord);
		if (queryCNameRes)
		{
			header->ID = htons(header->ID);
			memcpy(sendBuf, receiveBuffer, last);//����N���ֽڵ����ͻ�����
			unsigned short Flags = 0x8180;
			unsigned short Questions = (unsigned short)1;
			unsigned short Answer = (unsigned short)1;
			//printf("/*/*/*/%c*/*answer/*/\n", Answer);
			//printf("/*/*/*/%u*/*/answer*/\n", Answer);
			*(sendBuf + 3) = (char)Flags;
			*(sendBuf + 2) = Flags >> 8;
			*(sendBuf + 5) = (char)Questions;
			*(sendBuf + 4) = Questions >> 8;
			*(sendBuf + 7) = (char)Answer;
			*(sendBuf + 6) = Answer >> 8;

			cn_records_pro(*cnameRecord, sendBuf, &bytePos, 1);//cname��¼�����dns����������ʽ�����뷢�ͻ�����
			printf("���ش���%s������CNAME���ͼ�¼!\n\n", doName);
			//ת�����ͻ���
			
			sendto(serverSocket, sendBuf, bytePos, 0, (SOCKADDR*)&cli_ip, sizeof(SOCKADDR));
		}
		else
		{
			query_for_superior_server(receiveBuffer, header, cli_ip);//���һ���������������Ͳ�ѯ
		}
	}
	else if (tp == 2)//NS���Ͳ�ѯ
	{
		resRecord nsRecord[RESO_MAX];//�洢NS���ͼ�¼
		int queryNSResult = query_NS_record(db, zErrMsg, doName, str_len(doName), nsRecord);
		if (queryNSResult)
		{
			header->ID = htons(header->ID);
			memcpy(sendBuf, receiveBuffer, last);//����N���ֽڵ����ͻ�����

			//��QNAME�����ֵ�
			domainStore(QNAME, str_len(QNAME), 12, "");//12ΪQNAME��ʼ�ֽ���
			//printf("%s/*/*/*/%d*/*/*/\n", QNAME, str_len(QNAME));

			unsigned short Flags = 0x8180;
			unsigned short Questions = (unsigned short)1;
			unsigned short Answer = (unsigned short)(queryNSResult);
			//printf("/*/*/*/%c*/*answer/*/\n", Answer);
			//printf("/*/*/*/%u*/*/answer*/\n", Answer);
			*(sendBuf + 3) = (char)Flags;
			*(sendBuf + 2) = Flags >> 8;
			*(sendBuf + 5) = (char)Questions;
			*(sendBuf + 4) = Questions >> 8;
			*(sendBuf + 7) = (char)Answer;
			*(sendBuf + 6) = Answer >> 8;
			//printf("/*/*/*/%d/*%d/*/\n", sendBuf[6], sendBuf[7]);

			//cn_records_pro(*cnameRecord, sendBuf, &bytePos);//cname��¼�����dns����������ʽ�����뷢�ͻ�����

			ns_records_pro(nsRecord, queryNSResult, sendBuf, &bytePos);

			printf("���ش���%s������NS���ͼ�¼!\n\n", doName);
			//ת�����ͻ���
			sendto(serverSocket, sendBuf, bytePos, 0, (SOCKADDR*)&cli_ip, sizeof(SOCKADDR));
		}
		else
			query_for_superior_server(receiveBuffer, header, cli_ip);//���һ���������������Ͳ�ѯ
	}
	else if (tp == 15)//MX���Ͳ�ѯ
	{
		resRecord mxRecord[RESO_MAX];//�洢NS���ͼ�¼
		int queryMXResult = query_MX_record(db, zErrMsg, doName, str_len(doName), mxRecord);
		if (queryMXResult)
		{
			header->ID = htons(header->ID);
			memcpy(sendBuf, receiveBuffer, last);//����N���ֽڵ����ͻ�����

			//��QNAME�����ֵ�
			domainStore(QNAME, str_len(QNAME), 12, "");//12ΪQNAME��ʼ�ֽ���
			//printf("%s/*/*/*/%d*/*/*/\n", QNAME, str_len(QNAME));

			unsigned short Flags = 0x8180;
			unsigned short Questions = (unsigned short)1;
			unsigned short Answer = (unsigned short)(queryMXResult);
			//printf("/*/*/*/%c*/*answer/*/\n", Answer);
			//printf("/*/*/*/%u*/*/answer*/\n", Answer);
			*(sendBuf + 3) = (char)Flags;
			*(sendBuf + 2) = Flags >> 8;
			*(sendBuf + 5) = (char)Questions;
			*(sendBuf + 4) = Questions >> 8;
			*(sendBuf + 7) = (char)Answer;
			*(sendBuf + 6) = Answer >> 8;
			//printf("/*/*/*/%d/*%d/*/\n", sendBuf[6], sendBuf[7]);

			//cn_records_pro(*cnameRecord, sendBuf, &bytePos);//cname��¼�����dns����������ʽ�����뷢�ͻ�����

			mx_records_pro(mxRecord, queryMXResult, sendBuf, &bytePos);

			printf("���ش���%s������MX���ͼ�¼!\n\n", doName);
			//ת�����ͻ���
			sendto(serverSocket, sendBuf, bytePos, 0, (SOCKADDR*)&cli_ip, sizeof(SOCKADDR));
		}
		else
			query_for_superior_server(receiveBuffer, header, cli_ip);//���һ���������������Ͳ�ѯ
	}
	else
	{
		query_for_superior_server(receiveBuffer, header, cli_ip);//���һ���������������Ͳ�ѯ
	}
}

void resp_pro(dns_header *header, char *receiveBuffer)
{
	
	SOCKADDR_IN q_ip;//�˰�Ӧ�ظ����Ŀͻ���ip
	long ctime = GetTickCount();//��¼��ǰ���յ�����ʱ��
	int time_out = 0;//��¼�Ƿ��ǳ�ʱ��

	//��ԭDNS���ݰ�ͷID
	short int hid = header->ID;//ȡ����ID
	short int rehid;//�洢��ԭ����ID
	int i = 0;
	for (i = 0; i < it_length; i++)
	{//�ҵ�ID���±��ж�Ӧ���±�
		if (hid == new_id_table[i])  break;
	}
	if (i == it_length)
	{//Ϊ�ٵ�����ID�ѱ��滻��
		time_out = 1;
	}
	else if (ctime - timestamp[i] > 2000)
	{//Ϊ�ٵ�����IDδ���滻
		time_out = 1;
		for (int j = i; j < it_length - 1; j++)
		{//ɾ����ID���ݣ�����ID����ǰ��
			old_id_table[i] = old_id_table[i + 1];
			new_id_table[i] = new_id_table[i + 1];
			client_ip[i] = client_ip[i + 1];
		}
		it_length--;//���ȼ�1
	}
	else 
	{//�ǳ�ʱ��
		rehid = old_id_table[i];//ȡԭʼID
		q_ip = client_ip[i];
		for (int j = i; j < it_length - 1; j++)
		{//ɾ����ID���ݣ�����ID����ǰ��
			old_id_table[i] = old_id_table[i + 1];
			new_id_table[i] = new_id_table[i + 1];
			client_ip[i] = client_ip[i + 1];
		}
		it_length--;//���ȼ�1
		header->ID = rehid;//��ԭID
	}

	//ȡӦ�����ݣ��������ݿ�
	//ȡAA
	if ((header->FLAGS & 0x0400) == 0x0400) 
	{//��Ȩ��
		if (ddFlag)
		{
			printf("\n------------\n");
			printf("Ȩ��Ӧ��\n");
			printf("------------\n");
		}
	}
	else 
	{
		if (ddFlag)
		{
			printf("\n------------\n");
			printf("��Ȩ��Ӧ��\n");
			printf("------------\n");
		}
	}

	//�ж�RCODE
	if ((header->FLAGS & 0x000F) == 0x0003) 
	{//RCODEΪ3��ʾ��������
		//printf("δ��ѯ��������\n\n");
		//printf("Non-existent domain\n\n");
	}
	else 
	{
		if (ddFlag)
		{
			printf("Got answer:\n");
			printf("    HEADER:\n");
			printf("        opcode = ");
			if ((header->FLAGS & 0x7800) == 0x0000)
			{
				printf("Query, ");
			}
			else if ((header->FLAGS & 0x7800) == 0x0800)
			{
				printf("IQuery, ");
			}
			else if ((header->FLAGS & 0x7800) == 0x1000)
			{
				printf("Status, ");
			}
			else if ((header->FLAGS & 0x7800) == 0x2000)
			{
				printf("Notify, ");
			}
			else if ((header->FLAGS & 0x7800) == 0x2800)
			{
				printf("Update, ");
			}
			else
			{
				printf("Unassigned, ");
			}
			printf("id = %u, ", header->ID);
			printf("rcode = ");
			if ((header->FLAGS & 0x000F) == 0x0000)
			{
				printf("NoError\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0001)
			{
				printf("FormErr\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0002)
			{
				printf("ServFail\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0003)
			{
				printf("NXDomain\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0004)
			{
				printf("NotImp\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0005)
			{
				printf("Refused\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0006)
			{
				printf("YXDomain\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0007)
			{
				printf("YXRRSet\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0008)
			{
				printf("NXRRSet\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x0009)
			{
				printf("NotAuth\n");
			}
			else if ((header->FLAGS & 0x000F) == 0x000A)
			{
				printf("NotZone\n");
			}
			else
			{
				printf("Unassigned\n");
			}
			printf("        questions = %u, answers = %u, authority records = %u, additional = %u\n\n", header->QDCOUNT, header->ANCOUNT, header->NSCOUNT, header->ARCOUNT);
		}
		
		//������Ӧ��
		//printf("\n");
		//�ж������������Դ��¼����
		int ques = header->QDCOUNT;//query�ֶθ���
		int requ = header->ANCOUNT;//anwser�ֶθ���
		int aure = header->NSCOUNT;//authority�ֶθ���
		int adre = header->ARCOUNT;//additional�ֶθ���
		int reso = requ + aure + adre;//�ܵ���Դ��¼����

		int c_byte = sizeof(dns_header);//��ǰ��ֵ����ֽ�λ��
		//���Question Section����

		
		char doName[QNAME_MAX_LENTH];//��ѯ�������ʼ���ַ��׺��

		for (int i = 0; i < ques; i++)
		{
			/*if (i == 0)  printf("Question Section��%d������\n\n", ques);*/

			
			int length = 0;//�������ʼ���ַ��׺��ռ�õ��ֽ���
			unsigned short *type;//��ѯ����
			unsigned short *Class;//��ѯ��


			length = do_name_reso(0, 0, c_byte, doName, receiveBuffer);//�����������ʼ���ַ��׺��
			c_byte += length;
			type = (unsigned short*)(receiveBuffer + c_byte);//ȡ��ѯ����
			c_byte += 2;
			Class = (unsigned short*)(receiveBuffer + c_byte);//ȡClass
			c_byte += 2;

			*type = ntohs(*type);
			*Class = ntohs(*Class);
			//�����ѯ������Ϣ
			if (ddFlag)
			{
				printf("    QUESTIONS:\n");
				printf("        QName = %s\n", doName);
				printf("        QType = %d\n", *type);
				printf("        QClass = %d\n", *Class);
			}
			*type = htons(*type);
			*Class = htons(*Class);
		}

		//��ֺ�������Դ��¼���֣���Ϊ��ʽ��ͬ����ͬʱ����
		for (int i = 0; i < reso; i++)
		{
			//��������ĳ���ֶ�ʱ�������ʾ��Ϣ
			if (ddFlag)
			{
				
				if (i == 0)
				{
					printf("    ANSWERS:\n");
					printf("        number of Anwser Section = %d\n", requ);
				}
				if (i == requ)  printf("        number of Authority Records Section = %d\n", aure);
				if (i == requ + aure)  printf("        number of Additional Records Section = %d\n", adre);
			}

			char doname[QNAME_MAX_LENTH];//�������ʼ���ַ��׺��
			int length = 0;//�������ʼ���ַ��׺��ռ�õ��ֽ���
			unsigned short *type;//��ѯ����
			unsigned short *Class;//��ѯ��
			unsigned long *ttl;//����ʱ��
			unsigned short *relength;//��Դ���ݳ���

			//ȡ�����ֶ�
			length = do_name_reso(0, 0, c_byte, doname, receiveBuffer);//�����������ʼ���ַ��׺��
			c_byte += length;
			type = (unsigned short*)(receiveBuffer + c_byte);
			c_byte += 2;
			Class = (unsigned short*)(receiveBuffer + c_byte);
			c_byte += 2;
			ttl = (unsigned long*)(receiveBuffer + c_byte);
			c_byte += 4;
			relength = (unsigned short*)(receiveBuffer + c_byte);
			c_byte += 2;

			//��������ֶ���Ϣ
			*type = ntohs(*type);
			*Class = ntohs(*Class);
			*ttl = ntohl(*ttl);
			*relength = ntohs(*relength);
			if (ddFlag)
			{
				printf("    ->  Name = %s\n", doname);
				printf("        Type = %d\n", *type);
				printf("        Class = %d\n", *Class);
				printf("        TTL = %ld (%ld secs)\n", *ttl, *ttl);
			}

			char storeData[BUFFER_SIZE];
			char *zErrMsg = 0;

			int TTL = (int)*ttl;
			int ttlLen = (std::to_string(TTL)).length();
			int doNameLen = str_len(doName);
			int aliasLen = str_len(doname);
			int lenth[RESO_MAX];//������Դ��������	

			lenth[0] = doNameLen;//��������
			lenth[1] = aliasLen;//��������
			lenth[3] = 2;//class����
			lenth[4] = ttlLen;
			if (ddFlag)
			{
				printf("        DataLenth = %d\n", doNameLen);
				//printf("        TTL:%d\n", TTL);
			}
			if (*type == 1)
			{//IP��ַ����
				unsigned char ip_address[4];
				storeData[0] = 'A';
				storeData[1] = 'I';
				storeData[2] = 'N';
				storeData[4] = ip_address[0] = receiveBuffer[c_byte];
				storeData[5] = ip_address[1] = receiveBuffer[c_byte + 1];
				storeData[6] = ip_address[2] = receiveBuffer[c_byte + 2];
				storeData[7] = ip_address[3] = receiveBuffer[c_byte + 3];
				storeData[8] = '\0';
				if (ddFlag)
				{
					printf("        Internet Address = %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
				}
				
				string ip = translate_IP(ip_address);	
				int ipLen = ip.length();//ip��ַ����
				lenth[2] = 1;//type����
				lenth[5] = 1;//DataLength�ֶγ���
				lenth[6] = ipLen;
				const char *ipRes = ip.data();

				if(!query_A_record(db, zErrMsg,  doName, doNameLen, ipRes, ipLen))
				    insert_A_record(db, zErrMsg, doName, doname, storeData, storeData + 1, TTL, 4, ipRes, lenth);
			}
			else if (*type == 2)
			{//NS����
				lenth[2] = 2;//type���ͳ���Ϊ2
				storeData[0] = 'N';
				storeData[1] = 'S';
				storeData[2] = 'I';
				storeData[3] = 'N';

				char dname[QNAME_MAX_LENTH];//�洢����������������
				length = do_name_reso(0, 0, c_byte, dname, receiveBuffer);//��������������������
				
 				lenth[5] = (std::to_string(length)).length();
				lenth[6] = str_len(dname);
				if (ddFlag)
				{
					printf("        Name Length = %d\n", length);
					printf("        Name Server = %s\n", dname);
				}
				if (!query_NS_record(db, zErrMsg, doName, doNameLen, dname, lenth[6]))//������ݿ����޸�NS��¼��洢
					insert_NS_record(db, zErrMsg, doName, doname, storeData, storeData + 2, TTL, length, dname, lenth);
			}
			else if (*type == 5)
			{//CNAME����
				lenth[2] = 2;//type���ݳ���Ϊ2
				storeData[0] = 'C';//CN����
				storeData[1] = 'N';
				storeData[2] = 'I';//INclass
				storeData[3] = 'N';

				char cname[QNAME_MAX_LENTH];//�洢�淶��
				length = do_name_reso(0, 0, c_byte, cname, receiveBuffer);//�����淶��
				lenth[5] = (std::to_string(length)).length();
				lenth[6] = str_len(cname);
				if(!query_CNAME_record(db, zErrMsg, doName, doNameLen, cname, lenth[6]))//������ݿ����޸�CN��¼��洢
				  insert_CNAME_record(db, zErrMsg, doName, doname, storeData, storeData + 2, TTL, length, cname, lenth);
				if (ddFlag)
				{
					printf("        CName = %s\n", cname);
				}
			}
			else if (*type == 15)
			{//MX����
				lenth[2] = 2;//type���ͳ���Ϊ2
				storeData[0] = 'M';
				storeData[1] = 'X';
				storeData[2] = 'I';
				storeData[3] = 'N';


				char mname[QNAME_MAX_LENTH];//�洢�ʼ���������
				unsigned short *preference;//�ʼ������������ȼ�
				preference = (unsigned short*)(receiveBuffer + c_byte);
				length = do_name_reso(0, 0, c_byte + 2, mname, receiveBuffer);//�����ʼ���������
				*preference = ntohs(*preference);
				lenth[5] = (std::to_string(length)).length();
				lenth[6] = (std::to_string((int)*preference)).length();
				lenth[7] = str_len(mname);
				if (ddFlag)
				{
					printf("        Preference = %d\n", *preference);
					printf("        Mail Exchange = %s\n", mname);
				}
				if (!query_MX_record(db, zErrMsg, doName, doNameLen, mname, lenth[7]))//�ж����ݿ����Ƿ���MX��¼
					insert_MX_record(db, zErrMsg, doName, doname, storeData, storeData + 2, TTL, length + lenth[6], (int)*preference, mname, lenth);
				*preference = htons(*preference);
			}
			c_byte += *relength;//���������ֽ�������

			//ת���ֽ���
			*type = htons(*type);
			*Class = htons(*Class);
			*ttl = htonl(*ttl);
			*relength = htons(*relength);
		}
	}

	//ת���ֽ���
	
	header->ID = htons(header->ID);
	header->FLAGS = htons(header->FLAGS);
	header->QDCOUNT = htons(header->QDCOUNT);
	header->ANCOUNT = htons(header->ANCOUNT);
	header->NSCOUNT = htons(header->NSCOUNT);
	header->ARCOUNT = htons(header->ARCOUNT);

	//ת�����ͻ���
	if (time_out == 0)
	{//���ǳ�ʱ��
		sendto(serverSocket, receiveBuffer, last, 0, (SOCKADDR*)&q_ip, sizeof(SOCKADDR));
	}
}

int do_name_reso(int clength, int addlength, int c_byte, char doname[], char *receivebuffer)
{
	int length = clength;//��¼����ռ�ó���
	int alength = addlength;//��¼�����ĳ���
	int cu_byte = c_byte;
	unsigned char  c;

	c = receivebuffer[cu_byte];//ȡ��һ���������ֽ���
	while (c != 0)
	{//δ������������
		if ((c & 0xc0) == 0xc0)
		{//������ָ�룬�ѵ�ĩβ���̶�ռ��2�ֽڳ���
			unsigned short *x = (unsigned short *)(receivebuffer + cu_byte);
			*x = ntohs(*x);//ת��Ϊ�����ֽ���
			*x = (*x) & 0x3fff;//ǰ��bit����
			int offset = *x;
			int k = do_name_reso(length, alength, offset, doname, receivebuffer);//�ݹ�������洢������������ռ�ó���
			*x = (*x) | 0xc000;//ǰ��bit��ԭ
			*x = htons(*x);//��ԭΪ�����ֽ���
			return length + 2;//ָ��˵�������洢�ѵ�ĩβ������ռ�ó���
		}
		else
		{//����ָ��
			cu_byte++;
			length++;
			int le = c;//ת��Ϊ����
			for (int i = 0; i < le; i++)
			{
				//�洢�˿�����
				doname[alength++] = receivebuffer[cu_byte++];
				length++;
			}
			c = receivebuffer[cu_byte];//ȡ��һ���������ֽ���
			if (c != 0)  doname[alength++] = '.';
		}
	}
	cu_byte++;
	length++;//����������Ҳ����ռ�ó�����
	doname[alength] = '\0';
	return length;//��������ռ�ó���
}

void connect_string(char *a, char *b, int aLength, int bLength)//���ַ���b���ӵ��ַ���a��ĩ��
{
	int i;
	for (i = 0; i < bLength; i++)
	{
		a[aLength + i] = b[i];
	}
	a[aLength + i] = 0;
}

void connect_string(char *a, const char *b, int aLength, int bLength)//���ַ���b���ӵ��ַ���a��ĩ��
{
	int i;
	for (i = 0; i < bLength; i++)
	{
		a[aLength + i] = b[i];
	}
	a[aLength + i] = 0;
}

void insert_A_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, const char *Address, int *length)
{//�����ݿ��A_RECORD���в���һ������
	int sqlLength;
	char temSql[SQL_MAX] = "INSERT INTO A_RECORD (Name, Alias, Type, Class, Time_to_live, Data_length, Address) VALUES (";
	sqlLength = strlen(temSql);
	//temSql = temSql + "'" + domainName + "'" + ", " + "'" + ARecord + "'" + ", " + std::to_string(TTL) + ");";
	connect_string(temSql, "'", sqlLength, 1);
	sqlLength += 1;
	connect_string(temSql, Name, sqlLength, length[0]);
	sqlLength += length[0];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Alias, sqlLength, length[1]);
	sqlLength += length[1];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Type, sqlLength, length[2]);
	sqlLength += length[2];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Class, sqlLength, length[3]);
	sqlLength += length[3];
	connect_string(temSql, "', ", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, std::to_string(TTL).c_str(), sqlLength, length[4]);
	sqlLength += length[4];
	connect_string(temSql, ", ", sqlLength, 2);
	sqlLength += 2;
	connect_string(temSql, std::to_string(DataLength).c_str(), sqlLength, length[5]);
	sqlLength += length[5];
	connect_string(temSql, ", '", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, Address, sqlLength, length[6]);
	sqlLength += length[6];
	connect_string(temSql, "');", sqlLength, 3);
	int rc = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		//fprintf(stdout, "Operation done successfully\n");
	}

}

int query_A_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, const char *Address, int addLength)
{//����������Address��A_RECORD�в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from A_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "' and Address = '", sqlLength, 17);
	sqlLength += 17;
	connect_string(sql, Address, sqlLength, addLength);
	sqlLength += addLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		res++;
	}
	return res;
} 

int query_A_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records)
{//����������A_RECORD�в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from A_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		char* domainName1 = (char *)sqlite3_column_text(statement, 1);
		unsigned short type = 1;//��Դ����ΪA����
		unsigned short cla = 1;//CLASS�ֶ�ֵΪ1
		unsigned long TTL = (unsigned long)sqlite3_column_int(statement, 4);
		unsigned short dataLen = (unsigned short)sqlite3_column_int(statement, 5);
		char* addrRecord = (char *)sqlite3_column_text(statement, 6);
		int domainLen = str_len(domainName1);
		int addrLen = str_len(addrRecord);//ip��ַ�ĸ��ֽ�
		records[res].NAME = new char[domainLen + 1];
		memcpy(records[res].NAME, domainName1, domainLen);//����domainLen���ֽڵ�record-NAME
		records[res].NAME[domainLen] = '\0';
		records[res].TYPE = type;
		records[res].CLASS = cla;
		records[res].TTL = TTL;
		records[res].DATALENGTH = dataLen;
		records[res].RDATA = new char[addrLen + 1];
		memcpy(records[res].RDATA, addrRecord, addrLen);//����addrLen���ֽڵ�record-RDATA
		records[res].RDATA[addrLen] = '\0';

		res++;
	}
	return res;
}

void insert_CNAME_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, char *CNAME, int *length)
{//�����ݿ�ı�CNAME_RECORD�в���һ������
	int sqlLength;
	char temSql[SQL_MAX] = "INSERT INTO CNAME_RECORD (Name, Alias, Type, Class, Time_to_live, Data_length, CNAME) VALUES (";
	sqlLength = strlen(temSql);
	//temSql = temSql + "'" + domainName + "'" + ", " + "'" + ARecord + "'" + ", " + std::to_string(TTL) + ");";
	connect_string(temSql, "'", sqlLength, 1);
	sqlLength += 1;
	connect_string(temSql, Name, sqlLength, length[0]);
	sqlLength += length[0];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Alias, sqlLength, length[1]);
	sqlLength += length[1];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Type, sqlLength, length[2]);
	sqlLength += length[2];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Class, sqlLength, length[3]);
	sqlLength += length[3];
	connect_string(temSql, "', ", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, std::to_string(TTL).c_str(), sqlLength, length[4]);
	sqlLength += length[4];
	connect_string(temSql, ", ", sqlLength, 2);
	sqlLength += 2;
	connect_string(temSql, std::to_string(DataLength).c_str(), sqlLength, length[5]);
	sqlLength += length[5];
	connect_string(temSql, ", '", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, CNAME, sqlLength, length[6]);
	sqlLength += length[6];
	connect_string(temSql, "');", sqlLength, 3);
	//return;
	int rc = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		//fprintf(stdout, "Operation done successfully\n");
	}

}

int query_CNAME_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *record, int tag)
{//����������CNAME_RECORD�в�������
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from CNAME_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);

	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		if (res == 0)
		{
			char* domainName1 = (char *)sqlite3_column_text(statement, 1);
			unsigned short type = 5;//��Դ����ΪCNAME����
			unsigned short cla = 1;//CLASS�ֶ�ֵΪ1
			unsigned long TTL = (unsigned long)sqlite3_column_int(statement, 4);
			unsigned short dataLen = (unsigned short)sqlite3_column_int(statement, 5);
			char* cnameRecord = (char *)sqlite3_column_text(statement, 6);
			int domainLen = str_len(domainName1);
			int cnLen = str_len(cnameRecord);
			record->NAME = new char[domainLen + 1];
			memcpy(record->NAME, domainName1, domainLen);//����N���ֽڵ�record-NAME
			record->NAME[domainLen] = '\0';
			record->RDATA = new char[cnLen + 1];
			memcpy(record->RDATA, cnameRecord, cnLen);//����N���ֽڵ�record-RDATA
			record->RDATA[cnLen] = '\0';
			record->TYPE = type;
			record->CLASS = cla;
			record->TTL = TTL;
			record->DATALENGTH = dataLen;
		}
		res++;
	}
	return res;
}

int query_CNAME_record(sqlite3 *db, char *zErrMsg, char *Alias, int nameLength,  resRecord *record)
{//����������CNAME_RECORD�в�������
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from CNAME_RECORD where Alias = '";
	sqlLength = strlen(sql);
	connect_string(sql, Alias, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);

	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		char* domainName1 = (char *)sqlite3_column_text(statement, 1);
		unsigned short type = 5;//��Դ����ΪCNAME����
		unsigned short cla = 1;//CLASS�ֶ�ֵΪ1
		unsigned long TTL = (unsigned long)sqlite3_column_int(statement, 4);
		unsigned short dataLen = (unsigned short)sqlite3_column_int(statement, 5);
		char* cnameRecord = (char *)sqlite3_column_text(statement, 6);
		int domainLen = str_len(domainName1);
		int cnLen = str_len(cnameRecord);
		record->NAME = new char[domainLen + 1];
		memcpy(record->NAME, domainName1, domainLen);//����N���ֽڵ�record-NAME
		record->NAME[domainLen] = '\0';
		record->RDATA = new char[cnLen + 1];
		memcpy(record->RDATA, cnameRecord, cnLen);//����N���ֽڵ�record-RDATA
		record->RDATA[cnLen] = '\0';
		record->TYPE = type;
		record->CLASS = cla;
		record->TTL = TTL;
		record->DATALENGTH = dataLen;
		res++;
	}
	return res;
}

int query_CNAME_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *CNAME, int CNLength)
{//����������CNAME��CNAME_RECORD�в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from CNAME_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "' and CNAME = '", sqlLength, 15);
	sqlLength += 15;
	connect_string(sql, CNAME, sqlLength, CNLength);
	sqlLength += CNLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		char* domainName1 = (char *)sqlite3_column_text(statement, 0);
		char* CNAME_Record = (char *)sqlite3_column_text(statement, 6);

//		printf("domainName = %s\nCNAME = %s\n\n", domainName1, CNAME_Record);
		res++;
	}
	return res;
}

void insert_MX_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, int Preference, char *MX, int *length)
{//�����ݿ��MX_RECORD���в���һ������
	int sqlLength;
	char temSql[SQL_MAX] = "INSERT INTO MX_RECORD (Name, Alias, Type, Class, Time_to_live, Data_length, Preference, Mail_Exchange) VALUES (";
	sqlLength = strlen(temSql);
	//temSql = temSql + "'" + domainName + "'" + ", " + "'" + ARecord + "'" + ", " + std::to_string(TTL) + ");";
	connect_string(temSql, "'", sqlLength, 1);
	sqlLength += 1;
	connect_string(temSql, Name, sqlLength, length[0]);
	sqlLength += length[0];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Alias, sqlLength, length[1]);
	sqlLength += length[1];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Type, sqlLength, length[2]);
	sqlLength += length[2];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Class, sqlLength, length[3]);
	sqlLength += length[3];
	connect_string(temSql, "', ", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, std::to_string(TTL).c_str(), sqlLength, length[4]);
	sqlLength += length[4];
	connect_string(temSql, ", ", sqlLength, 2);
	sqlLength += 2;
	connect_string(temSql, std::to_string(DataLength).c_str(), sqlLength, length[5]);
	sqlLength += length[5];
	connect_string(temSql, ", ", sqlLength, 2);
	sqlLength += 2;
	connect_string(temSql, std::to_string(Preference).c_str(), sqlLength, length[6]);
	sqlLength += length[6];
	connect_string(temSql, ", '", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, MX, sqlLength, length[7]);
	sqlLength += length[7];
	connect_string(temSql, "');", sqlLength, 3);
	int rc = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		//fprintf(stdout, "Operation done successfully\n");
	}

}

void insert_NS_record(sqlite3 *db, char *zErrMsg, char *Name, char *Alias, char *Type, char *Class, int TTL, int DataLength, char *NS, int *length)
{//�����ݿ��NS_RECORD���в���һ������
	int sqlLength;
	char temSql[SQL_MAX] = "INSERT INTO NS_RECORD (Name, Alias, Type, Class, Time_to_live, Data_length, Name_Server) VALUES (";
	sqlLength = strlen(temSql);
	//temSql = temSql + "'" + domainName + "'" + ", " + "'" + ARecord + "'" + ", " + std::to_string(TTL) + ");";
	connect_string(temSql, "'", sqlLength, 1);
	sqlLength += 1;
	connect_string(temSql, Name, sqlLength, length[0]);
	sqlLength += length[0];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Alias, sqlLength, length[1]);
	sqlLength += length[1];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Type, sqlLength, length[2]);
	sqlLength += length[2];
	connect_string(temSql, "', '", sqlLength, 4);
	sqlLength += 4;
	connect_string(temSql, Class, sqlLength, length[3]);
	sqlLength += length[3];
	connect_string(temSql, "', ", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, std::to_string(TTL).c_str(), sqlLength, length[4]);
	sqlLength += length[4];
	connect_string(temSql, ", ", sqlLength, 2);
	sqlLength += 2;
	connect_string(temSql, std::to_string(DataLength).c_str(), sqlLength, length[5]);
	sqlLength += length[5];
	connect_string(temSql, ", '", sqlLength, 3);
	sqlLength += 3;
	connect_string(temSql, NS, sqlLength, length[6]);
	sqlLength += length[6];
	connect_string(temSql, "');", sqlLength, 3);
	int rc = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		//fprintf(stdout, "Operation done successfully\n");
	}

}

int query_MX_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records)
{//����������MX_RECORD�в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from MX_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	

	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		char* domainName1 = (char *)sqlite3_column_text(statement, 1);
		unsigned short type = 15;//��Դ����ΪNS����
		unsigned short cla = 1;//CLASS�ֶ�ֵΪ1
		unsigned long TTL = (unsigned long)sqlite3_column_int(statement, 4);
		unsigned short dataLen = (unsigned short)sqlite3_column_int(statement, 5);
		unsigned short preference = (unsigned short)sqlite3_column_int(statement, 6);
		char* mxServer = (char *)sqlite3_column_text(statement, 7);
		int domainLen = str_len(domainName1);
		int mxServerLen = str_len(mxServer);//NS��¼�ĳ���
		records[res].NAME = new char[domainLen + 1];
		memcpy(records[res].NAME, domainName1, domainLen);//����domainLen���ֽڵ�record-NAME
		records[res].NAME[domainLen] = '\0';
		records[res].TYPE = type;
		records[res].CLASS = cla;
		records[res].TTL = TTL;
		records[res].DATALENGTH = dataLen;
		records[res].RDATA = new char[mxServerLen + 1];
		memcpy(records[res].RDATA, mxServer, mxServerLen);//����addrLen���ֽڵ�record-RDATA
		records[res].RDATA[mxServerLen] = '\0';
		records[res].PREFERENCE = preference;
		res++;
	}
	return res;
}

int query_MX_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *mName, int mNameLen)
{//����������MX��MX_RECORD�в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from MX_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "' and Mail_Exchange = '", sqlLength, 23);
	sqlLength += 23;
	connect_string(sql, mName, sqlLength, mNameLen);
	sqlLength += mNameLen;

	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		res++;
	}
	return res;
}

int query_NS_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, char *NAME_SERVER, int nameServerLen)
{//����������NS�����ݿ��в�ѯ����
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from NS_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "' and NAME_SERVER = '", sqlLength, 21);
	sqlLength += 21;
	connect_string(sql, NAME_SERVER, sqlLength, nameServerLen);
	sqlLength += nameServerLen;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		res++;
	}
	return res;
}

int query_NS_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength, resRecord *records)
{//����������NS_RECORD�в�������
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from NS_RECORD where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		//char* domainName1 = (char *)sqlite3_column_text(statement, 0);
		//char* NameServer = (char *)sqlite3_column_text(statement, 6);
		char* domainName1 = (char *)sqlite3_column_text(statement, 1);
		unsigned short type = 2;//��Դ����ΪNS����
		unsigned short cla = 1;//CLASS�ֶ�ֵΪ1
		unsigned long TTL = (unsigned long)sqlite3_column_int(statement, 4);
		unsigned short dataLen = (unsigned short)sqlite3_column_int(statement, 5);
		char* nameServer = (char *)sqlite3_column_text(statement, 6);
		int domainLen = str_len(domainName1);
		int nameServerLen = str_len(nameServer);//NS��¼�ĳ���
		records[res].NAME = new char[domainLen + 1];
		memcpy(records[res].NAME, domainName1, domainLen);//����domainLen���ֽڵ�record-NAME
		records[res].NAME[domainLen] = '\0';
		records[res].TYPE = type;
		records[res].CLASS = cla;
		records[res].TTL = TTL;
		records[res].DATALENGTH = dataLen;
		records[res].RDATA = new char[nameServerLen + 1];
		memcpy(records[res].RDATA, nameServer, nameServerLen);//����addrLen���ֽڵ�record-RDATA
		records[res].RDATA[nameServerLen] = '\0';
		res++;
	}
	return res;
}

int query_undesirable_web_record(sqlite3 *db, char *zErrMsg, char *Name, int nameLength)
{//����������UNDESIRABLE_WEB_TABLE�в�������
	int ret = 0;
	sqlite3_stmt *statement;
	int sqlLength;
	char sql[SQL_MAX] = "SELECT * from UNDESIRABLE_WEB where Name = '";
	sqlLength = strlen(sql);
	connect_string(sql, Name, sqlLength, nameLength);
	sqlLength += nameLength;
	connect_string(sql, "';", sqlLength, 2);
	sqlite3_prepare(db, sql, -1, &statement, NULL);
	if (ret != SQLITE_OK)
	{
		printf("prepare error ret : %d\n", ret);
		return 0;
	}
	int res = 0;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		char* address = (char *)sqlite3_column_text(statement, 1);
		res++;
		//printf("address = %s\n", address);
	}
	return res;
}

void delete_expired_data(sqlite3 *db, char *zErrMsg)
{//����ɾ�����ݿ��г�ʱ�ļ�¼
	int ret = 0;
	sqlite3_stmt *statement;
	time_t a1 = time(NULL);
	while (1)
	{
		mt.lock();
		//printf("passed %d second(s)\n", (int)(time(0) - a1));
		char sql[SQL_MAX] = "SELECT Name, Time_to_live, Address, Time_Stamp from A_RECORD";
		ret = sqlite3_prepare(db, sql, -1, &statement, NULL);
		if (ret != SQLITE_OK)
		{
			printf("prepare error ret : %d\n", ret);
			return;
		}
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			char* Name = (char *)sqlite3_column_text(statement, 0);
			int TTL = sqlite3_column_int(statement, 1);
			char* Address = (char *)sqlite3_column_text(statement, 2);
			char* timeStamp = (char *)sqlite3_column_text(statement, 3);
			//printf("%s %d %s %s\n", Name, TTL, Address, timeStamp);
			tm tmTimeStamp = { 0,0,0,0,0,0,0,0 };
			sscanf_s(timeStamp, "%d-%d-%d %d:%d:%d", &tmTimeStamp.tm_year, &tmTimeStamp.tm_mon, &tmTimeStamp.tm_mday, \
				&tmTimeStamp.tm_hour, &tmTimeStamp.tm_min, &tmTimeStamp.tm_sec);
			tmTimeStamp.tm_year -= 1900;
			tmTimeStamp.tm_mon--;
			time_t timeStampToSec = mktime(&tmTimeStamp);
			time_t currentTime = time(NULL);
			long timeDifferece = (long)(currentTime - timeStampToSec);
			if (timeDifferece >= TTL)
			{
				int temSqlLength;
				char temSql[SQL_MAX] = "delete from A_RECORD where Name = '";
				temSqlLength = strlen(temSql);
				connect_string(temSql, Name, temSqlLength, strlen(Name));
				temSqlLength += strlen(Name);
				connect_string(temSql, "' and Address = '", temSqlLength, 17);
				temSqlLength += 17;
				connect_string(temSql, Address, temSqlLength, strlen(Address));
				temSqlLength += strlen(Address);
				connect_string(temSql, "';", temSqlLength, 2);

				ret = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
				if (ret != SQLITE_OK)
				{
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				else
				{
					//fprintf(stdout, "delete %s %s from A_RECORD\n", Name, Address);
				}
			}

		}


		for (int i = 0; i < SQL_MAX; i++)
		{
			sql[i] = 0;
		}
		connect_string(sql, "SELECT Name, Time_to_live, CNAME, Time_Stamp from CNAME_RECORD", 0, 62);

		ret = sqlite3_prepare(db, sql, -1, &statement, NULL);
		if (ret != SQLITE_OK)
		{
			printf("prepare error ret : %d\n", ret);
			return;
		}
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			char* Name = (char *)sqlite3_column_text(statement, 0);
			int TTL = sqlite3_column_int(statement, 1);
			char* CNAME = (char *)sqlite3_column_text(statement, 2);
			char* timeStamp = (char *)sqlite3_column_text(statement, 3);
			tm tmTimeStamp = { 0,0,0,0,0,0,0,0 };
			sscanf_s(timeStamp, "%d-%d-%d %d:%d:%d", &tmTimeStamp.tm_year, &tmTimeStamp.tm_mon, &tmTimeStamp.tm_mday, \
				&tmTimeStamp.tm_hour, &tmTimeStamp.tm_min, &tmTimeStamp.tm_sec);
			tmTimeStamp.tm_year -= 1900;
			tmTimeStamp.tm_mon--;
			time_t timeStampToSec = mktime(&tmTimeStamp);
			time_t currentTime = time(NULL);
			long timeDifferece = (long)(currentTime - timeStampToSec);
			if (timeDifferece >= TTL)
			{
				int temSqlLength;
				char temSql[SQL_MAX] = "delete from CNAME_RECORD where Name = '";
				temSqlLength = strlen(temSql);
				connect_string(temSql, Name, temSqlLength, strlen(Name));
				temSqlLength += strlen(Name);
				connect_string(temSql, "' and CNAME = '", temSqlLength, 15);
				temSqlLength += 15;
				connect_string(temSql, CNAME, temSqlLength, strlen(CNAME));
				temSqlLength += strlen(CNAME);
				connect_string(temSql, "';", temSqlLength, 2);

				ret = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
				if (ret != SQLITE_OK)
				{
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				else
				{
					//fprintf(stdout, "delete %s %s from CNAME_RECORD\n", Name, CNAME);
				}
			}

		}


		for (int i = 0; i < SQL_MAX; i++)
		{
			sql[i] = 0;
		}
		connect_string(sql, "SELECT Name, Time_to_live, Mail_Exchange, Time_Stamp from MX_RECORD", 0, 67);

		ret = sqlite3_prepare(db, sql, -1, &statement, NULL);
		if (ret != SQLITE_OK)
		{
			printf("prepare error ret : %d\n", ret);
			return;
		}
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			char* Name = (char *)sqlite3_column_text(statement, 0);
			int TTL = sqlite3_column_int(statement, 1);
			char* MX = (char *)sqlite3_column_text(statement, 2);
			char* timeStamp = (char *)sqlite3_column_text(statement, 3);
			tm tmTimeStamp = { 0,0,0,0,0,0,0,0 };
			sscanf_s(timeStamp, "%d-%d-%d %d:%d:%d", &tmTimeStamp.tm_year, &tmTimeStamp.tm_mon, &tmTimeStamp.tm_mday, \
				&tmTimeStamp.tm_hour, &tmTimeStamp.tm_min, &tmTimeStamp.tm_sec);
			tmTimeStamp.tm_year -= 1900;
			tmTimeStamp.tm_mon--;
			time_t timeStampToSec = mktime(&tmTimeStamp);
			time_t currentTime = time(NULL);
			long timeDifferece = (long)(currentTime - timeStampToSec);
			if (timeDifferece >= TTL)
			{
				int temSqlLength;
				char temSql[SQL_MAX] = "delete from MX_RECORD where Name = '";
				temSqlLength = strlen(temSql);
				connect_string(temSql, Name, temSqlLength, strlen(Name));
				temSqlLength += strlen(Name);
				connect_string(temSql, "' and Mail_Exchange = '", temSqlLength, 23);
				temSqlLength += 23;
				connect_string(temSql, MX, temSqlLength, strlen(MX));
				temSqlLength += strlen(MX);
				connect_string(temSql, "';", temSqlLength, 2);

				ret = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
				if (ret != SQLITE_OK)
				{
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				else
				{
					//fprintf(stdout, "delete %s %s from MX_RECORD\n", Name, MX);
				}
			}

		}


		for (int i = 0; i < SQL_MAX; i++)
		{
			sql[i] = 0;
		}
		connect_string(sql, "SELECT Name, Time_to_live, Name_Server, Time_Stamp from NS_RECORD", 0, 65);

		ret = sqlite3_prepare(db, sql, -1, &statement, NULL);
		if (ret != SQLITE_OK)
		{
			printf("prepare error ret : %d\n", ret);
			return;
		}
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			char* Name = (char *)sqlite3_column_text(statement, 0);
			int TTL = sqlite3_column_int(statement, 1);
			char* NS = (char *)sqlite3_column_text(statement, 2);
			char* timeStamp = (char *)sqlite3_column_text(statement, 3);
			tm tmTimeStamp = { 0,0,0,0,0,0,0,0 };
			sscanf_s(timeStamp, "%d-%d-%d %d:%d:%d", &tmTimeStamp.tm_year, &tmTimeStamp.tm_mon, &tmTimeStamp.tm_mday, \
				&tmTimeStamp.tm_hour, &tmTimeStamp.tm_min, &tmTimeStamp.tm_sec);
			tmTimeStamp.tm_year -= 1900;
			tmTimeStamp.tm_mon--;
			time_t timeStampToSec = mktime(&tmTimeStamp);
			time_t currentTime = time(NULL);
			long timeDifferece = (long)(currentTime - timeStampToSec);
			if (timeDifferece >= TTL)
			{
				int temSqlLength;
				char temSql[SQL_MAX] = "delete from NS_RECORD where Name = '";
				temSqlLength = strlen(temSql);
				connect_string(temSql, Name, temSqlLength, strlen(Name));
				temSqlLength += strlen(Name);
				connect_string(temSql, "' and Name_Server = '", temSqlLength, 21);
				temSqlLength += 21;
				connect_string(temSql, NS, temSqlLength, strlen(NS));
				temSqlLength += strlen(NS);
				connect_string(temSql, "';", temSqlLength, 2);

				ret = sqlite3_exec(db, temSql, callback, 0, &zErrMsg);
				if (ret != SQLITE_OK)
				{
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				else
				{
					//fprintf(stdout, "delete %s %s from NS_RECORD\n", Name, NS);
				}
			}

		}
		mt.unlock();
		Sleep(DELETE_INTERVAL);
	}
}