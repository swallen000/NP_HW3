#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 2048

using namespace std;

typedef struct type{
	char buf[BUFSIZE];
	int now;
	int size;
	int byte;
	bool send;
	//bool done;
	char file[1024];
	char name[50];
}TCP;

int main(int argc,char *argv[]){
	if(argc!=4){
		cout<<"usage: ./client <SERVER_IP> <SERVER_PORT> <NAME>";
		exit(1);
	}
	
	int sockfd=0;
	sockfd = socket(PF_INET , SOCK_STREAM , 0);
	if(sockfd < 0){
		cout<<"Fail to create a socket";
		exit(1);
	}

	char ip[1024];
	strcpy(ip,argv[1]);
	struct sockaddr_in info;
	memset(&info , 0 , sizeof(info));
	info.sin_family = PF_INET;
	if( !isalpha(ip[0]) ) 
		info.sin_addr.s_addr = inet_addr(argv[1]);
	else{
		struct hostent* host;
		host=gethostbyname(argv[1]);
		info.sin_addr.s_addr = *(unsigned long *)host->h_addr;
	}
	info.sin_port = htons(atoi(argv[2]));
	//getnameinfo(&info, sizeof(sa), argv[1], sizeof(argv[1]), service, sizeof service, 0);

	if(connect(sockfd , (struct sockaddr *)&info , sizeof(info))<0){
		cout<<"Fail to connect";
		exit(1);
	}	

	fd_set rfds,mas;
	FD_ZERO(&rfds);
	FD_ZERO(&mas);
	FD_SET(0,&mas);
	FD_SET(sockfd,&mas);
	char buf[1024];
	FILE *file;

	char temp2[BUFSIZE];
	string name=argv[3];
	bool flag=false;
	memcpy(temp2,name.c_str(),name.length());
	temp2[strlen(temp2)]='\0';
	//cout<<"my name is : "<<temp2<<endl;
	if(!flag){
		send(sockfd , temp2 , strlen(temp2)+1 , 0);
		//cout<<"not receive\n";
		recv(sockfd , temp2 , BUFSIZE , 0);
		//cout<<"now receive\n";
		flag=true;
	}
	//sleep(1);
	while(1){
		rfds = mas;
		if(select(sockfd+1 , &rfds , NULL , NULL , NULL)<0){
			cout<<"fail to select";
			exit(1);
		}

		for(int i=0 ; i<=sockfd ; i++){
			if(FD_ISSET(i , &rfds)){
				memset(buf , 0 , 1024);
				if(i==0){
					fgets(buf , 1024 , stdin);
					if(strncmp(buf , "/exit" , 5) == 0){
						FD_CLR(0 , &rfds);
						exit(0);
					}
					else if( strncmp(buf , "/put " ,5)==0 ){
						TCP tmp;
						memcpy(tmp.buf , buf , strlen(buf));
						tmp.buf[strlen(buf)]='\0';
						send(sockfd , &tmp ,sizeof(type) ,0);
						//send(sockfd , "*" , 1 ,0);
						//sleep(1);
						FILE *file;
						int round=1;
						string str,filename;
						str=buf+5;
						istringstream ss(str);
						ss>>filename;
						cout<<"Uploading file : "<<filename<<endl;
						cout<<"Progress : [";
						file=fopen( filename.c_str(),"rb");
						int packet,size;
						fseek(file , 0 ,SEEK_END);
						size=ftell(file);
						fseek(file , 0 ,SEEK_SET);
						if( (size%BUFSIZE)!=0 )
							packet=(size/BUFSIZE)+1;
						else
							packet=(size/BUFSIZE);
						//cout<<"packet size : "<<packet<<endl;
						TCP *s;
						s=new TCP[packet];
						for(int i=0 ; i<packet ; i++){
							s[i].send=false;
							s[i].now=i;
							s[i].size=packet;
							//s[i].file=(char *)filename.c_str();
							//cout<<s[i].file<<endl;
						}
						for(int i=0;i<packet;i++){
							int byte;
							byte=fread(s[i].buf , sizeof(char) , BUFSIZE , file);
							s[i].byte=byte;
							//cout<<"now read "<<i<<endl;
							
							if(byte<BUFSIZE)
								s[i].buf[byte]='\0';
							//cout<<s[i].buf<<endl;
						}
						while(true){
							for(int i=0;i<packet;i++){
								if(i>(round*packet/10)){
									cout<<"#";
									round++;
								}
								if(s[i].send==false){
									write(sockfd , &s[i] , sizeof(type));
									s[i].send=true;
									usleep(10000);
								}
							}
							break;
						}
						cout<<"]\nUpload "<<filename<<" complete!\n";
						delete [] s;
						fclose(file);
						sleep(1);
						break;
					}
					else if( strncmp(buf , "/sleep " ,7)==0){
						string str;
						int second;
						str=buf+7;
						istringstream ss(str);
						ss>>second;
						for(int i=1 ; i<=second ; i++){
							cout<<"Sleep "<<i<<endl;
							sleep(1);
						}
						cout<<"Client wakes up\n";
					}
					else {
						int test;
						TCP tmp;
						memcpy(tmp.buf , buf , strlen(buf));
						tmp.buf[strlen(buf)]='\0';
						test=send(sockfd , tmp.buf , strlen(buf)+1 , 0);
						if(test<0){
							cout<<"Fail to send";
							exit(1);
						}
					}
				}
				else{
					//cout<<"into server send\n";
					int  setup=0;
					bool done=true;
					bool digit=true;
					int lastone=0;
					int last_size;
					int count=0;
					int byte;
					int round=1;
					string f;
					buf[BUFSIZE];
					while(true){
						memset(buf,0,BUFSIZE);
						byte=recv(sockfd , buf , BUFSIZE , 0);
						/*if(strlen(buf)!=2048)
							buf[strlen(buf)]='\0';*/
						//cout<<"begin byte : "<<byte<<endl;
						//cout<<"begin strlen : "<<strlen(buf)<<endl;
						//cout<<r.buf[0]<<r.buf[1]<<endl;
						//cout<<"here\n";
						//cout<<buf<<endl;
						
						//cout<<"now : "<<r.now<<endl;
						if(byte<=0 ){
							cout<<"error!\n";
							exit(1);
						}
						for(int i=0;i<strlen(buf);i++){
							if(!isdigit(buf[i])){
								digit=false;
								break;
							}
						}
						/*if(digit==true)
							cout<<"now digit is true \n";*/
						if(digit||!done){
							if(setup==0){
								//cout<<"start to receive file\n";
								
								
								//cout<<"open file success\n";
								//cout<<"size : ";
								/*for(int i=0;i<strlen(buf);i++)
									cout<<buf[i];*/
								//cout<<endl;
								lastone=atoi(buf);
								//cout<<"last one = "<<lastone<<endl;
								done=false;
								setup++;
							}
							else if(setup==1){
								//cout<<"last one size : ";
								/*for(int i=0;i<strlen(buf);i++)
									cout<<buf[i];*/
								//cout<<endl;
								last_size=atoi(buf);
								//cout<<"check : "<<last_size<<endl;
								setup++;
							}
							else if(setup==2){
								f=buf;
								cout<<"Downloading file : "<<buf<<endl<<"Progress : [";
								/*for(int i=0;i<strlen(buf);i++)
									cout<<buf[i];*/
								//cout<<endl<<"Progress : [";
								file=fopen(buf,"wb+");
								setup++;
							}
							else{
								if(strncmp(buf,"done",4)==0){
									cout<<"]\n";
									cout<<"Download "<<f<<" complete!\n";
									done=true;
									fclose(file);
									break;
								}
								if( count>=(round*lastone)/10 ){
									cout<<"#";
									round++;
								}
								/*if(lastone)
									fwrite(r.buf,sizeof(char),last_size,file);
								else if(!digit)*/
								if(count==lastone-1)
									fwrite(buf,sizeof(char),last_size,file);
								else
									fwrite(buf,sizeof(char),byte,file);
								//fwrite(buf , sizeof(char) , strlen(buf) , file);
								//if(count>4870){

								/*if(count==lastone-1){
									/cout<<"this is last one write "<<last_size<<endl;
									cout<<"now count = "<<count<<" lastone = "<<lastone<<endl;
								}*/
									//cout<<"receive "<<count<<endl;
									//cout<<"byte : "<<byte<<endl;
								//}
								count++;
							}
							digit=true;
						}
						else{
							cout<<buf;
							break;
						}
						//cout<<"#########################\n############\n";
						/*if((r.buf[0]=='W'&&r.buf[1]=='e'&&r.buf[2]=='l')||r.buf[0]=='E'&&r.buf[1]=='r'&&r.buf[2]=='r'){
							cout<<"message : "<<r.buf;
							break;
						}
						if(r.buf[0]=='d'&&r.buf[1]=='o'&&r.buf[2]=='n'){
							fclose(file);
							break;
						}*/
						/*if(r.now==-1){
							cout<<"just message!\n";
							cout<<r.buf;
							break;
						}*/
						//cout<<"byte:"<<byte<<endl;
						
						/*if(temp[r.now].send==false){
							//if(r.now>1440||r.now<5)
							//cout<<"receive total package size : "<<byte<<" bytes\n";
								//cout<<"receive "<<r.now<<" successfully\n";
							temp[r.now].send=true;
							//cout<<"now : "<<r.now<<" byte : "<<r.byte<<endl;
							memcpy(temp[r.now].buf,r.buf,r.byte);
							if(r.byte<BUFSIZE-1){
								cout<<"into not enough\n";
								cout<<"now count : "<<count<<" size : "<<r.size<<endl;
								temp[r.now].buf[r.byte]='\0';
							}
							temp[r.now].byte=r.byte;
							temp[r.now].size=r.size;
							//cout<<temp[r.now].buf<<endl;
							count++;
							//cout<<"now count = "<<count<<endl;
						}*/
						//if(count==r.size){
						//	cout<<"download finish!\n##################"<<endl;
						//	for(int i=0;i<r.size;i++){
							
						//	}
						//	cout<<"write finish!\n";
						//	delete [] temp;
							//fclose(file);
							//break;
						//}	

					}
				}
			}
		}
	}
	close(sockfd);
	return 0;
}