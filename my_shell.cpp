#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include <pwd.h>
#include<string.h>
#include<string>
#include<time.h>
#include<termios.h>
#include<vector>  
#include <sys/stat.h>
#include <fcntl.h>
#define UP_KEY 1001
#define DOWN_KEY 1004
#define RIGHT_KEY 1003
#define LEFT_KEY 1002
#define TAB_KEY 9
#define BACKSP_KEY 127
#define ENTER_KEY 10
using namespace std;
/*
 ....................increase the buffer size of the char* onece done to be on the safe side............................
 * */
//-------------------------------------------- Global variables----------------- -----------------------
int mypaths_len;
char* mypaths;//will be taken from file
int myhome_len;
char* myhome;//home
int user_len;
char* myuser;//done
char* myhostname;//done
int host_name_len;
char* myps_u;//will be taken from file.
int myps_len;
char* myps_s;//will be taken from file.

void fill_param(){
	//PS for u and s and path is already written in the file.
	//username hostname and myhome needs to be read from the system.
	//------------------------------------- taking data from the systems ------------------------------
	//----- reading host name------------------------
	host_name_len=100;
	myhostname=(char*)malloc((host_name_len+1)*sizeof(char));
	if(gethostname(myhostname,host_name_len)==-1){
		printf("error in reading host name\n");
	}
	//------------------------------------------------
	//---- reading user name -------------------------
	struct passwd* temp=getpwuid(getuid());
	user_len=100;
	myuser=(char*)malloc((user_len+1)*(sizeof(char)));
	strcpy(myuser,temp->pw_name);
	//------------------------------------------------
	//----- reading home dir ------------------------
	myhome_len=100;
	myhome=(char*)malloc((myhome_len+1)*sizeof(char));
	strcpy(myhome,temp->pw_dir);
	//-----------------------------------------------
	//----------------------------------------reading from file ----------------------------------
	myps_len=100;
	myps_u=(char*)malloc((myps_len+1)*(sizeof(char)));
	myps_s=(char*)malloc((myps_len+1)*(sizeof(char)));
	mypaths_len=300;
	mypaths=(char*)malloc((mypaths_len+1)*(sizeof(char)));
	FILE* fp;
	char* line=NULL;
	size_t len=0;
	ssize_t read;
	fp=fopen(".myshellinfo","r");
	if(fp==NULL){
		printf("error in reading the file\n");
	}
	int line_number=0;	
	while((read=getline(&line,&len,fp))!=-1){
		//printf("retriving line %s",line);
		if(line_number==0)
			strcpy(myps_u,line);
		else if(line_number==1)
			strcpy(myps_s,line);
		else if(line_number==2)
			strcpy(mypaths,line);
		else
			break;

		line_number++;
	}
	fclose(fp);
	if(line)
		free(line);
	//----------------------done reading----------------------
	

	int tempp=strlen(mypaths);
	mypaths[tempp-1]='\0';
	tempp=strlen(myps_u);
	myps_u[tempp-1]='\0';
	tempp=strlen(myps_s);
	myps_s[tempp-1]='\0';
	//-------------setting the enviorment varable-------------
	setenv("PATH",mypaths,1);	//only this enviorment variable is required to set.
	setenv("HOME",myhome,1);
	setenv("USER",myuser,1);
	char* p=(char*)malloc(200*sizeof(char));
	//char *getcwd(char *buf, size_t size);
	p=getcwd(p,200);
	if(p==NULL){
		printf("error in reading present working directory\n");
		free(p);
	}else{
		setenv("PWD",p,1);
		free(p);
	}
	//--------------------------------------------------------	
	
}

//this method will be backbone of our program.
//moving the terminal to the background will not be done in this method we will assume the terminal is already in background.
//i.e the file discriptor of the terminal program will already be manneged.
int exe_fg(char* argv[],int input_fd,int output_fd){
	pid_t p=fork();
	if(p==0){//the code path of the child process.
		//now learn dup first.
		int a;
		a=dup2(input_fd,0);
		if(a==-1){
			printf("error in hand fd");
			return -1;
		}
		a=dup2(output_fd,1);
		if(a==-1){
			printf("error in hand fd 2");
			return -1;
		}
		a=execvp(*argv,argv);//this may through erro
		if(a==-1){
			printf("error in executing the command\n");
			exit(1);
			return -1;
		}
	}else{
		wait(NULL);
	}
	return 0;
}
/*
int exe_fgp(char* argv[],int input_fd,int output_fd){
		int a;
		a=dup2(0,input_fd);
		if(a==-1){
			printf("error in hand fd");
			return -1;
		}
		dup2(1,output_fd);
		if(a==-1){
			printf("error in hand fd 2");
			return -1;
		}
		a=execvp(*argv,argv);//this may through erro
		if(a==-1){
			printf("error in executing the command\n");
			exit(1);
			return -1;
		}
	
	return 0;
}
*/

//required to change he directory.
int my_cd(char** p){
 	int a=chdir(p[1]);
	if(a==-1){
		printf("error in changing directory\n");
		return -1;
	}
	//we have to again set the enviorment variable.
	char* pw=(char*)malloc(200*sizeof(char));
        pw=getcwd(pw,200);
        if(pw==NULL){
                printf("error in reading present working directory\n");
		free(pw);
        }else{
                setenv("PWD",pw,1);
		free(pw);
        }
	return 0;
 }

char getkey(){
	int ch;
	struct termios orig_t;
	struct termios new_t;
	tcgetattr(fileno(stdin),&orig_t);//setting the terminal in raw mode
	memcpy(&new_t,&orig_t,sizeof(struct termios));
	new_t.c_lflag &= ~(ICANON|ECHO);//bitwise and oprator.//remove not echo    ECHO
	new_t.c_cc[VTIME]=0;
	new_t.c_cc[VMIN]=0;
	tcsetattr(fileno(stdin),TCSANOW,&new_t);
	while((ch=fgetc(stdin))==EOF);
	tcsetattr(fileno(stdin),TCSANOW,&orig_t);
	return ch;
}
int get_key(){//the key 
	char ch[3];
	ch[0]=getkey();
	if(ch[0]==27){
		ch[1]=getkey();
		ch[2]=getkey();
		/*1001 for up 
		1002 for left 
		1003 for right and 
		1004 for down
		9 is for tap
		*/
		if(ch[2]==65){//up
			return 1001;
		}else if(ch[2]==66){//down
			return 1004;
		}else if(ch[2]==68){//left
			return 1002;
		}else if(ch[2]==67){//right
			return 1003;
		}
	}else{
		return ch[0];
	}

}

int howmany(char* s,char c){
	int len=strlen(s);
	int i;
	int count=0;
	for(i=0;i<len;i++){
		if(s[i]==c){
			count++;
		}
	}
	return count+1;
}

void chr_print(char** a){
	int i=0;
	while(a[i]){
		printf("%s \n",a[i]);
		i++;
	}
}
/*
bool ischarpp(char** arr,char c,int n){
	int i;
	int j;
	for(j=0;j<n;j++){
		int len=strlen(arr[j]);
		for(i=0;i<len;i++){
			if(arr[j][i]==c){
				return true;
			}
		}
	}
	return false;
}
int my_str_len(char* arr){
	int i;
	i=0;
	while(arr[i]!='\0'){
		i++;
		//printf(" %d ",arr[i]);
	}
	//printf(" %d ",arr[i]);
	printf("\n");
	return i;
}*/
bool is_there(char* arr,char c){
		int i;
		int len=strlen(arr);
		for(i=0;i<len;i++){
			if(arr[i]==c){
				return true;
			}
		}
	return false;
}
char** token_machine(char* s,int* num,char t,bool flag){//return number of token //it will create the space also.
	char z[2];
	z[0]=t;
	z[1]='\0';
	char dub_buffer[1024];
	strcpy(dub_buffer,s);
	//printf("bufferd copyed %s\n",dub_buffer);
	int arr_num=howmany(s,t);
	//printf("number of arr_num %d\n",arr_num);
				char** cmd=new char*[arr_num+1];
				char* token=strtok(dub_buffer,z);
				int j;
				int piece_len=strlen(s);
				for(j=0;j<arr_num;j++){
					cmd[j]=new char[piece_len];
				}
				j=0;
				while(token!=NULL){
					strcpy(cmd[j],token);
					//printf("printing the vaue of token %s",cmd[j]);
					j++;
					token=strtok(NULL,z);
				}
				//printf("rturning th valu of j that is %d\n",j);
				if(flag){
					cmd[j]=0;
					j++;
				}
				*num=j;
			return cmd;
}
void pipe_handel(char*** d,int l,int f_i,int f_o){
	int f1=open(".marvel",O_RDWR|O_CREAT,0777);
	int f2=open(".dc",O_RDWR|O_CREAT,0777);
	ftruncate(f2,0);
	ftruncate(f1,0);
    bool flag;
	lseek(f1,0,SEEK_SET);
	exe_fg(d[0],0,f1);
	flag=true;
    int i;
    for(i=1;i<l-1;i++){
		if(flag){
			ftruncate(f2,0);
			lseek(f1,0,SEEK_SET);
			exe_fg(d[i],f1,f2);
			flag=false;
		}else{
			ftruncate(f1,0);
			lseek(f2,0,SEEK_SET);
			exe_fg(d[i],f2,f1);
			flag=true;
		}
	}
	if(flag){
		lseek(f1,0,SEEK_SET);
		exe_fg(d[l-1],f1,1);
	}else{
		lseek(f2,0,SEEK_SET);
		exe_fg(d[l-1],f2,1);
	}
    close(f1);
    close(f2);
	int w=remove(".marvel");
	if(w==-1){
		printf("error in deleting the file 1\n");
	}
	w=remove(".dc");
	if(w==-1){
		printf("error in deleting the file 2\n");
	}
}
//exe_fgp(d[c],a[0],a[1]);
//pipe_handel(cmd_pipe_arr,cmd_pipe_len,0,0,1,-1);
/*
void pipe_handel(char*** d,int l,int f_i,int f_o){
	int** pipe_arr=new int*[l];
	int i;
	for(i=0;i<l;i++){
		pipe_arr[i]=new int[2];
		pipe(pipe_arr[i]);
	}
	for(i=0;i<l;i++){
		int t=fork();
		if(t==0){//child process
			if(i==0){
				dup2(0,f_i);
				dup2(1,pipe_arr[0][1]);
			}
			if(i!=0&&i!=l-1){
				dup2(0,pipe_arr[i-1][1]);
				dup2(pipe_arr[i][0],pipe_arr[i-1][1]);
				dup2(1,pipe_arr[i][1]);
			}
			if(i==l-1){
				dup2(0,pipe_arr[i-1][1]);
				dup2(pipe_arr[i][0],pipe_arr[i-1][1]);
				dup2(1,f_o);
			}
			//closing all the other pipe
			int j;
			for(j=0;j<l;j++){
				//if(j!=i){
					close(pipe_arr[j][0]);
					close(pipe_arr[j][1]);
				//}
			}
			int a=execvp(d[i][0],d[i]);
		}else{//parent process

		}
	}
	for(i=0;i<l;i++){
		wait(NULL);	
	}
	int j;
	for(j=0;j<l;j++){
		close(pipe_arr[j][0]);
		close(pipe_arr[j][1]);
	}
}*/
int main(){

	//handeling signals
	signal(SIGINT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	//filling the enviorment variables
	fill_param();
	vector<string> his;
	int index=0;
	char input_buffer[1024];
	char dup_input_buffer[1024];
	int i;


	for(i=0;i<1023;i++){
		input_buffer[i]='a';
	}
	input_buffer[i]='\0';
	
	while(strcmp(input_buffer,"exit")!=0){
		printf("%s ",myps_u);
		i=0;
		input_buffer[i]='\0';
		int a;
		int key_count=0;




		while(1){//(a=get_key())!=ENTER_KEY
			a=get_key();
			//printf("the value of a recorded is %d\n",a);
			if(a==TAB_KEY){

			}else if(a==UP_KEY){//goes to previous history
				// empty string poppin segmentation fault and key count recalibration needs to be done.
				if(index>=0){
					int k;
					//int len=primary_stack.top().size();
					int len=his[index].size();
					for(k=0;k<len;k++){
						input_buffer[k]=his[index][k];
					}
					input_buffer[k]='\0';
					for(k=0;k<key_count;k++){
						printf("\b");
						printf(" ");
						printf("\b");
					}
					printf("%s",input_buffer);
					key_count=strlen(input_buffer);
					i=key_count;
					index--;
					if(index<0){
						index++;
					}
				}
			}else if(a==DOWN_KEY){//goes to next history
				if(index<his.size()){
					int k;
					//int len=primary_stack.top().size();
					int len=his[index].size();
					for(k=0;k<len;k++){
						input_buffer[k]=his[index][k];
					}
					input_buffer[k]='\0';
					for(k=0;k<key_count;k++){
						printf("\b");
						printf(" ");
						printf("\b");
					}
					printf("%s",input_buffer);
					key_count=strlen(input_buffer);
					i=key_count;
					index++;
					if(index==his.size()){
						index--;
					}
				}
			}else if(a==BACKSP_KEY){
				if(key_count>0){
				printf("\b");
				printf(" ");
				printf("\b");
				i--;
				key_count--;
				}
			}else if(a==LEFT_KEY){// this needs to be handeled in detail.
				i--;
				printf("\b");
				key_count--;
			}else if(a==ENTER_KEY){
				printf("\n");
				//prinf("enter key routine executed");
				break;
			}else{//the main condition to be handeled
				printf("%c",(char)a);
				input_buffer[i]=(char)a;
				i++;
				key_count++;
			}
		}
		input_buffer[i]='\0';//this needs to be placed in the cache history stack.
		//printf("is things working coorectly %s ho yes\n",input_buffer);






		index=his.size()-1;
		if(his.empty()||strcmp(input_buffer,his[index].c_str())!=0){
			his.push_back(input_buffer);
			index++;
		}
		if(strcmp(input_buffer,"")==0){
			continue;
		}
		if(strcmp(input_buffer,"exit")==0){
			exit(1);
		}
		//input buffer ready here








		//----------------- tokening the stuff---------------
		int arr_num=howmany(input_buffer,' ');
		char** cmd=new char*[arr_num];
		strcpy(dup_input_buffer,input_buffer);
		char* token = strtok(dup_input_buffer, " ");
		int j;
		int piece_len=strlen(input_buffer);
		for(j=0;j<arr_num;j++){
			cmd[j]=new char[piece_len];
		}  
		j=0;
   		while (token != NULL) { 
			strcpy(cmd[j],token);
			j++;
        	token = strtok(NULL, " "); 	
   		}
		   cmd[j]=0;//ther are j command in th cmd
		//-------- token available in cmd ------------------------








		//--------- processing the token--------------------------
		if(strcmp(cmd[0],"cd")==0){
			my_cd(cmd);
		}else if(strcmp(cmd[0],"exit")==0){
		
		}else{//normal command
			if(is_there(input_buffer,'|')){
				char** cmd_pipe;
				int cmd_pipe_len;
				cmd_pipe=token_machine(input_buffer,&cmd_pipe_len,'|',false);
				//cmd_pipe[i] comands are there. chr_print(cmd_pipe);
				char*** cmd_pipe_arr=new char**[cmd_pipe_len];	//
				int r;
				for(r=0;r<cmd_pipe_len;r++){
					int temp;
					cmd_pipe_arr[r]=token_machine(cmd_pipe[r],&temp,' ',true);
				}
				pipe_handel(cmd_pipe_arr,cmd_pipe_len,0,1);
			}else{
				exe_fg(cmd,0,1);
			}
		}
		//--------------------------------------------------------
		//---------deallocatin the space-------------------------
		int k;
		for(k=0;k<j;k++){
			delete cmd[j];
		}
			delete cmd;
		//---------------------------------------------------------  
	}

	return 0;
}
