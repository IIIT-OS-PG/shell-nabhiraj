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
#include<dirent.h>
#include <map>
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
char* pwd;
map<string,string> allias_map;
vector<string> his;
vector<string> exe_file_name;
map<string,string> app_info;
int recent_fg_exit_status;
int howmany(char* s,char c);
char** token_machine(char* s,int* num,char t,bool flag);

void fill_param(){
	//PS for u and s and path is already written in the file.
	//username hostname and myhome needs to be read from the system.
	//------------------------------------- taking data from the systems ------------------------------
	pwd=(char*)malloc(1024*sizeof(char));
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
		if(line_number==0){
			strcpy(myps_u,line);
		}else if(line_number==1){
			strcpy(myps_s,line);
		}else if(line_number==2){
			strcpy(mypaths,line);
		}else{
			//we need tonek method above this.
			int nn;
			char** temp=token_machine(line,&nn,' ',false);
			app_info[temp[0]]=temp[1];
			//printf("%s\n",temp[0]);
			//printf("%s\n",temp[1]);
			//deallocating the space
			int i;
			for(i=0;i<nn;i++){
				delete temp[i];
			}
			delete temp;
			//deallocation done.
		}
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
		strcpy(pwd,p);
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
		//we have to reactivate the signals here....
		signal(SIGINT,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
		signal(SIGQUIT,SIG_DFL);
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
		if(strcmp(argv[0],"history")==0){ //if error comes remove this if condition.
			int i;
			for(i=0;i<his.size();i++){
				printf("%d %s\n",i+1,his[i].c_str());
			}
			exit(0);
		}else{
			a=execvp(*argv,argv);//this may through erro
			if(a==-1){
				printf("error in executing the command\n");
				exit(1);
				return -1;
			}
		}
	}else{
		wait(&recent_fg_exit_status);
		char* ts=new char[10];
		sprintf(ts,"%d",recent_fg_exit_status);
		allias_map["$?"]=ts;
		delete[] ts;
	}
	return 0;
}
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
				strcpy(pwd,pw);
				allias_map["$PWD"]=pwd;
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
	exe_fg(d[0],f_i,f1);
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
		exe_fg(d[l-1],f1,f_o);
	}else{
		lseek(f2,0,SEEK_SET);
		exe_fg(d[l-1],f2,f_o);
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


int ifcointain(char* b,char* a){//if  a is in b
	int al=strlen(a);
	int bl=strlen(b);
	int i,j;
	for(i=0;i<=bl-al;i++){
		for(j=0;j<al;j++){
			if(b[i+j]!=a[j]){
				break;
			}
		}
		if(j==al){
			return i;
		}
	}
	return -1;
}
void insert_string(char* s,char* j,char* t){//if string j is present replace it with string t
        int start_index;
        int sl=strlen(s);
        int sj=strlen(j);//string to be replaced
        int st=strlen(t);//the new string.
        while((start_index=ifcointain(s,j))!=-1){
                s[start_index]='\0';
                char* temp=s+start_index+sj;
                int l=strlen(temp);
                char* temp2=new char[l+2];
                strcpy(temp2,temp);
                strcat(s,t);
                strcat(s,temp2);
                delete[] temp2;
        }
}
bool only_single(char* s,char ss){
	int i;
	int len=strlen(s);
	for(i=0;i<len;i++){
		if(s[i]!=ss){
			return false;
		}
	}
	return true;
}

//----------------------------- implimentation of tri ------------------------------------------------------
class node{
    public:
    node* children[256];//all ascii charecter.
    bool is_end;
    string till_now;
    node(string t,bool i){
        till_now = t;
        is_end=i;
        int ii;
        for(ii=0;ii<256;ii++){
            children[ii]=NULL;
        }
    }
	node(){
		is_end=false;
		int ii;
        for(ii=0;ii<256;ii++){
            children[ii]=NULL;
        }
	}
};
void insert_tri(node* root,string key){
    node* temp=root;
	//temp=new node();
    int i;
    for(i=0;i<key.length();i++){
		int index=key[i];
        if(!temp->children[index]){
            temp->children[index]=new node(key.substr(0,i+1),false);
        }
        temp=temp->children[index];
    }
    temp->is_end=true;
}
node* node_of_intrest(node* root,string key){
    node* temp=root;
    int i;
	//printf("entering inside the loop\n");
    for(i=0;i<key.length();i++){
		//printf("index number %d\n",i);
		//printf("%c\n",key[i]);
        int index=key[i];
		//printf("the valu of index is %c\n",index);
        if(!temp->children[index]){
			//printf("insde the if condition\n");
            return NULL;
        }else{
			//printf("inside the elese condition\n");
            temp=temp->children[index];
        }
    }
    return temp;
}
void print_all(node* root){
    int i;
    for(i=0;i<256;i++){
        if(root->children[i]!=NULL){
            if(root->children[i]->is_end){
                printf("%s\n",root->children[i]->till_now.c_str());
            }
           // printf("calling another print all\n");
            print_all(root->children[i]);
        }
    }
}
void print_matching(node* root,string s){//this is the mehod which will be called in the main.
	//printf("control entring\n");
    node* t=node_of_intrest(root,s);
	//printf("spidrman\n");
    if(t!=NULL){
		//printf("control entering in and not null\n");
        print_all(t);
	}else{
		//printf("control not entering because null\n");
	}
}
//----------------------------------------------------------------------------------------------------------------------------------

int main(){

	//handeling signals
	signal(SIGINT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	//filling the enviorment variables
	fill_param();
	//inserting the enviorment variabls.
	recent_fg_exit_status=0;
	//store the path to the shell.
	char* inisial_path=new char[100];
	strcat(inisial_path,pwd);
	strcat(inisial_path,"/newtonian_shell");
	allias_map["$SHELL"]=inisial_path;
	allias_map["$PATH"]=mypaths;
	allias_map["$HOME"]=myhome;
	allias_map["$USER"]=myuser;
	allias_map["$HOSTNAME"]=myhostname;
	allias_map["$PS1"]=myps_u;
	allias_map["$PWD"]=pwd;
	allias_map["~"]=myhome;
	int my_pid=getpid();
	char spid[10];
	sprintf(spid, "%d",my_pid);
	allias_map["$$"]=spid;
	allias_map["$0"]="newtonian_shell";
	allias_map["$?"]="0";
	//--------------------- data stuctures for application handeling-----------------
	 
	//-------------------------------------------------------------------------------
	node* my_tri=new node();
	int num_p;
	char** temp=token_machine(mypaths,&num_p,':',false);
	int i;
	for(i=0;i<num_p;i++){
		struct dirent* d;
		DIR* dr=opendir(temp[i]);
		if(dr!=NULL){
			while((d=readdir(dr))!=NULL){
				if(d->d_type==DT_REG)
					exe_file_name.push_back(d->d_name);
			}
		}
	}
	//now we will insert every thing in the tri.

	for(i=0;i<exe_file_name.size();i++){
		insert_tri(my_tri,exe_file_name[i]);
	}//all tri data inserted.
	i=0;
	int script_fd=-987; // file discriptor for scripting file. and its default value.
	//vector<string> his;
	int index=0;
	bool first_time=false;
	char input_buffer[1024];
	char dup_input_buffer[1024];
	for(i=0;i<1023;i++){
		input_buffer[i]='a';
	}
	input_buffer[i]='\0';
	bool tri_flag=false;	
	while(strcmp(input_buffer,"exit")!=0){
		printf("%s ",myps_u);
		i=0;
		int max=0;
		input_buffer[i]='\0';
		int a;
		int key_count=0;

		if(tri_flag){
			tri_flag=false;
		}


		while(1){//(a=get_key())!=ENTER_KEY
			a=get_key();
			//printf("the value of a recorded is %d\n",a);
			if(a==TAB_KEY){
					input_buffer[i]='\0';
					printf("\n");
					print_matching(my_tri,input_buffer);
					tri_flag=true;
					break;
			}else if(a==UP_KEY){//goes to previous history
				// empty string poppin segmentation fault and key count recalibration needs to be done.
				if(index>=0&&first_time){
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
					max--;
				}
			}else if(a==LEFT_KEY){// this needs to be handeled in detail.
			if(key_count>0){
					printf("\b");
					printf(" ");
					printf("\b");
					i--;
					key_count--;
					max--;
				}
			}else if(a==RIGHT_KEY){
				if(i<max){
					printf("%c",input_buffer[i]);
					i++;
					key_count++;
				}
			}else if(a==ENTER_KEY){
				printf("\n");
				//prinf("enter key routine executed");
				break;
			}else{//the main condition to be handeled
				printf("%c",(char)a);
				input_buffer[i]=(char)a;
				i++;
				key_count++;
				max++;
			}
		}
		input_buffer[i]='\0';//this needs to be placed in the cache history stack.
		//printf("is things working coorectly %s ho yes\n",input_buffer);

		if(tri_flag){
			continue;
		}




		index=his.size()-1;
		if(his.empty()||strcmp(input_buffer,his[index].c_str())!=0){
			if(!only_single(input_buffer,' ')){
				his.push_back(input_buffer);
				index++;
			}else{
				continue;
			}
		}
		if(strcmp(input_buffer,"")==0){
			continue;
		}
		if(strcmp(input_buffer,"exit")==0){
			exit(1);
		}
		//input buffer ready here
		//here we will do aliasing and all.
		map<string,string>::iterator itr;
		for(itr=allias_map.begin();itr!=allias_map.end();itr++){
			string key=itr->first;
			int len_k=key.size();
			string value=itr->second;
			int len_v=value.size();
			int i;
			char* key_c=new char[len_k+1];
			char* value_c=new char[len_v+1];
			for(i=0;i<len_k;i++){
				key_c[i]=key[i];
			}
			key_c[i]='\0';
			for(i=0;i<len_v;i++){
				value_c[i]=value[i];
			}
			value_c[i]='\0';
			insert_string(input_buffer,key_c,value_c);
			//deallocating the memory
			delete[] key_c;
			delete[] value_c;
		}
		//----------------- tokening the stuff---------------
		int buf_len=strlen(input_buffer);//length of the input buffer
		int arr_num=howmany(input_buffer,' ');
		char** cmd=new char*[arr_num];
		strcpy(dup_input_buffer,input_buffer); //copying of input buffr to duplicate buffer is done here.
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
		//printf("control must be coming till here\n");
		bool to_deloc=false;
		int totalenv_len;
		char** enev_temp;
		enev_temp=NULL;
		if(is_there(cmd[0],'=')){
			enev_temp=token_machine(input_buffer,&totalenv_len,'=',false);
			to_deloc=true;
		//	printf("delocation bit set to 1\n");
		}

		//if(strcmp(input_buffer,"history")==0){
		//	strcpy(input_buffer,"cat ");
		//}
		if(script_fd!=-987){
			char* buf=new char[buf_len];
			strcpy(buf,input_buffer);
			write(script_fd,buf,buf_len);
			char c='\n';
			write(script_fd,&c,1);
			delete[] buf;
		}
		//--------- processing the token--------------------------

		if(strcmp(cmd[0],"open")==0){
			//we will have to change the cmd[0];
			//extract the extention of cmd[1].
			if(is_there(cmd[1],'.')){
			//delete cmd[0];
			int ttt;
			char** temppp=token_machine(cmd[1],&ttt,'.',false);
			//temppp[1];
			delete cmd[0];
			cmd[0]=new char[100];
			strcpy(cmd[0],app_info[temppp[1]].c_str());
			int lll=app_info[temppp[1]].size();
			cmd[0][lll-1]='\0';
			}else{
				printf("wrong command\n");
				continue;
			}
			//printf("this is %s",cmd[0]);
			//printf("cmd[1] is %s",cmd[1]);
			//printf("the value of j is %d",j);
		}

		
		if(strcmp(cmd[0],"cd")==0){
			my_cd(cmd);
		}else if((j==2)&&(strcmp(cmd[0],"record")==0)&&(strcmp(cmd[1],"start")==0)){
			//printf("inside the record condition\n");
			if(script_fd==-987){
				script_fd=open("recode_file_2019201062.txt",O_RDWR|O_CREAT|O_APPEND,0777);
				if(script_fd==-1){
					printf("error in opening scripting file");
					script_fd=-987;
				}
			}

		}else if((j==2)&&(strcmp(cmd[0],"record")==0)&&(strcmp(cmd[1],"stop")==0)){
			//printf("insidee the record stop condtion\n");
			if(script_fd!=-987){
				close(script_fd);
				script_fd=-987;
			}
		}else if(strcmp(cmd[0],"exit")==0){
			//printf("inside the record stop condition\n");
			if(script_fd!=-987){
				close(script_fd);
				script_fd=-987;
			}
		}else if(strcmp(cmd[0],"alias")==0){//this is the allias routine
			//printf("inside the alias condition\n");
			int alaias_len;
			char** alias_arr=token_machine(cmd[1],&alaias_len,'=',false);
			allias_map[alias_arr[0]]=alias_arr[1];
			//deallocatin the memory.
			int i;
			for(i=0;i<alaias_len;i++){
				delete alias_arr[i];
			}
			delete alias_arr;
		}else if((j==1)&&(enev_temp!=NULL)&&(strcmp(enev_temp[0],"PATH")==0||strcmp(enev_temp[0],"HOME")==0||strcmp(enev_temp[0],"USER")==0||strcmp(enev_temp[0],"HOSTNAME")==0||strcmp(enev_temp[0],"PS1")==0||strcmp(enev_temp[0],"PWD")==0)){
			//printf("inside the enviorment variable condition\n");
			//int totalenv_len;
			//char** enev_temp=token_machine(input_buffer,&totalenv_len,'=',false);
			//allias_map[enev_temp[0]]=enev_temp[1];
			setenv(enev_temp[0],enev_temp[1],1);
			//printf("the value of enev_temp[1] is %s %s",enev_temp[0],enev_temp[1]);
			if(strcmp(enev_temp[0],"PATH")==0){
				//mypaths=enev_temp[1];
				strcpy(mypaths,enev_temp[1]);	
				allias_map["$PATH"]=enev_temp[1];
			}else if(strcmp(enev_temp[0],"HOME")==0){
				//myhome=enev_temp[1];
				strcpy(myhome,enev_temp[1]);
				allias_map["$HOME"]=enev_temp[1];
				allias_map["~"]=myhome;
			}else if(strcmp(enev_temp[0],"USER")==0){
				//myuser=enev_temp[1];
				strcpy(myuser,enev_temp[1]);
				allias_map["$USER"]=enev_temp[1];
			}else if(strcmp(enev_temp[0],"HOSTNAME")==0){
				//myhostname=enev_temp[1];
				strcpy(myhostname,enev_temp[1]);
				allias_map["$HOSTNAME"]=enev_temp[1];
			}else if(strcmp(enev_temp[0],"PS1")==0){
				//printf("the value of enev_temp is %s\n",enev_temp[1]);
				//printf("the value of enev_temp is %s\n",myps_u);
				allias_map["$PS1"]=enev_temp[1];
				//myps_u=enev_temp[1];
				strcpy(myps_u,enev_temp[1]);				
				//printf("the value of enev_temp is %s\n",allias_map["$PS1"]);
			}else if(strcmp(enev_temp[0],"PWD")==0){
				//pwd=enev_temp[1];
				allias_map["$PWD"]=enev_temp[1];
			}else{
				printf("this condition is should not occur\n");
			}
			//int i;
			//for(i=0;i<totalenv_len;i++){
			//	delete enev_temp[i];
			//}
			//delete enev_temp;
		}else{//normal command
			//printf("inside the normal condition\n");
			int input_disc=0;
			int output_disc;
			bool append_or_not; //true for append, false for not append. 
			//caching redirection................... and setting the input output file discriptor
			if(is_there(input_buffer,'>')){
				int g=howmany(input_buffer,'>');
				g--;
				if(g==2){
					append_or_not=true;
				}else if(g==1){
					append_or_not=false;
				}
				//there are j command in cmd array
				char* file_name=cmd[j-1];
				int y;
				for(y=0;y<j;y++){
					if(is_there(cmd[y],'>')){
						cmd[y]=0;
						break;
					}
				}
				//removing '>>' from input buffer
				int ii;
				int input_length=strlen(input_buffer);
				for(ii=0;ii<input_length;ii++){
					if(input_buffer[ii]=='>'){
						input_buffer[ii]='\0';
					}
				}
				struct stat buffer;
				int exist=stat(file_name,&buffer);
				if(exist!=0){//file do not exist
					if(append_or_not){
						printf("file does not exist not a valid command\n");
						continue;	
					}
					output_disc=open(file_name,O_WRONLY|O_CREAT,0777);
				}else{
					if(append_or_not){
						output_disc=open(file_name,O_WRONLY|O_APPEND,0777);
					}else{
						output_disc=open(file_name,O_WRONLY,0777);
						//we hav to trucate th file
						ftruncate(output_disc,0);
					}
				}
			}else{
				output_disc=1;
			}
			//printf("redirection calculated\n");
			if(is_there(input_buffer,'|')){
			//	printf("dealing with pipe\n");
				char** cmd_pipe;
				int cmd_pipe_len;
				cmd_pipe=token_machine(input_buffer,&cmd_pipe_len,'|',false);
				char*** cmd_pipe_arr=new char**[cmd_pipe_len];
				int r;
				int* temp=new int[cmd_pipe_len];
				for(r=0;r<cmd_pipe_len;r++){
					cmd_pipe_arr[r]=token_machine(cmd_pipe[r],&temp[r],' ',true);
				}
				pipe_handel(cmd_pipe_arr,cmd_pipe_len,input_disc,output_disc);
				//now we should handel the memory freeing.
				for(r=0;r<cmd_pipe_len;r++){
					delete cmd_pipe[r];
				}
				delete cmd_pipe;
				for(r=0;r<cmd_pipe_len;r++){
					for(int r2=0;r2<temp[r];r2++){
						delete cmd_pipe_arr[r][r2];
					}
					cmd_pipe_arr[r];
				}
				delete cmd_pipe_arr;
			}else{
				//printf("no need to deal with pipe\n");
				//printing the value of cmd
				exe_fg(cmd,input_disc,output_disc);//most normal condition
			}

				if(output_disc!=1){
			//		printf("closing the output disc\n");
					close(output_disc);
				}


		}
		//--------------------------------------------------------
		//---------deallocatin the space-------------------------
		//printf("deallocation old space\n");
		int k;
		//printf("deallocatin here\n");
		/*
		for(k=0;k<j;k++){
			delete cmd[j];
		}
			delete cmd;
			*/
		//---------------------------------------------------------  
		//int i;
			//for(i=0;i<totalenv_len;i++){
			//	delete enev_temp[i];
			//}
			//delete enev_temp;
			//printf("deallocation new shit\n");
		if(to_deloc){
			//printf("deallocating space\n");
			to_deloc=false;
			//printf("deallocation bit set back\n");
			int i;
			printf("deallocatin here a\n");
			for(i=0;i<totalenv_len;i++){
				delete enev_temp[i];
			}
			delete enev_temp;
		}
		first_time=true;
	}

	return 0;
}
