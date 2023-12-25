/*
 * Copyright (C) 2022-present Shanghai HPC-NOW Technologies Co., Ltd.
 * This code is distributed under the license: MIT License
 * Originally written by Zhenrong WANG
 * mailto: zhenrongwang@live.com | wangzhenrong@hpc-now.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <malloc.h>
#include <conio.h> // This header is not standard! Only for mingw.
#elif __linux__
#include <malloc.h>
#include <sys/time.h>
#elif __APPLE__
#include <sys/time.h>
#endif

#include "now_macros.h"
#include "now_md5.h"
#include "general_funcs.h"

char command_flags[CMD_FLAG_NUM][16]={
    "-b", // batch mode, skip every confirmation
    "-r", // recursive
    "-rf", // recursive + force
    "-f", // force
    "--all", 
    "--list",
    "--force",
    "--recursive",
    "--print", // print contents
    "--installed",
    "--verbose",
    "--read", // read contents
    "--edit",
    "--std", // standard info
    "--err", // error info
    "--dbg", // tf dbg info
    "--this", // this
    "--hist", // historical
    "--mc", // rebuild mc
    "--mcdb", //rebuild mcdb
    "--bkey", // display bucket passwd
    "--rkey", // display root passwd
    "--admin", //export admin privilege
    "--accept", // accept license terms
    "--echo", //echo_flag
    "--od",
    "--month",
    "--gcp",
    "--rdp",
    "--copypass"
};

char command_keywords[CMD_KWDS_NUM][32]={
    "-c", //cluster
    "-u", //user
    "-p", // password
    "-s", //Source  | start
    "-e", //End
    "-d", //Destination
    "-t", // target
    "-n", //node_name
    "--cmd",
    "--dcmd",
    "--ucmd",
    "--acmd",
    "--app",
    "--jcmd",
    "--jname", //Job Name
    "--jid",
    "--jtime",
    "--jexec",
    "--jdata",
    "--level",
    "--log",
    "--vol",
    "--cname", //cluster_name
    "--ak",
    "--sk",
    "--az-sid",
    "--az-tid",
    "--ul", // user list
    "--key", //key file
    "--rg",
    "--az",
    "--nn", //node_num
    "--tn", //tasks per node
    "--un", //user_num
    "--mi",
    "--ci",
    "--os",
    "--ht",
    "--inst",
    "--repo",
    "--conf",
    "--hloc",
    "--cloc",
    "--hver",
    "--dbg-level",
    "--max-time",
    "--tf-run"
};

int string_to_positive_num(char* string){
    int i,sum=0;
    int length=strlen(string);
    if(length<1){
        return -1;
    }
    for(i=0;i<length;i++){
        if(*(string+i)<'0'||*(string+i)>'9'){
            return -1;
        }
    }
    for(i=0;i<length;i++){
        sum=sum*10+(*(string+i)-'0');
    }
    return sum;
}


int get_key_value(char* filename, char* key, char ch, char* value){
    char line_buffer[LINE_LENGTH_SHORT]="";
    char head[128]="";
    char tail[256]="";
    FILE* file_p=fopen(filename,"r");
    if(file_p==NULL){
        strcpy(value,"");
        return -1;
    }
    if(strlen(key)==0||ch=='\0'){
        fclose(file_p);
        strcpy(value,"");
        return -3;
    }
    while(!feof(file_p)){
        fgetline(file_p,line_buffer);
        reset_string(tail);
        get_seq_string(line_buffer,ch,1,head);
        get_seq_string(line_buffer,ch,2,tail);
//        printf("%s\t%s\t%s\n",line_buffer,head,tail);
        if(strcmp(key,head)==0){
            fclose(file_p);
            strcpy(value,tail);
            return 0;
        }
    }
    fclose(file_p);
    strcpy(value,"");
    return 1;
}

void reset_string(char* orig_string){
    int length=strlen(orig_string);
    int i;
    for(i=0;i<length;i++){
        *(orig_string+i)='\0';
    }
}
/* 
 * Potential risk: If the line_string array is not long enough, there might be overflow! The users should make sure the defined line_string is long enough
 * and the actual file doesn't contain lines longer than the LINE_LENGTH macro!
 * This function works, but not general or perfect at all! 
 * SEGMENT FAULT MAY OCCUR IF YOU DO NOT USE THIS FUNCTION PROPERLY!
 */
int fgetline(FILE* file_p, char* line_string){
//    char ch;
    int ch='\0';
    int i=0;
    if(file_p==NULL){
        return -1;
    }
    reset_string(line_string);
    do{
        ch=fgetc(file_p);
        if(ch!=EOF&&ch!='\n'){
            *(line_string+i)=ch;
            i++;
        }
    }while(ch!=EOF&&ch!='\n'&&i!=LINE_LENGTH); // Be careful! This function can only handle lines <= 4096 chars. Extra chars will be ommited
    if(i==LINE_LENGTH){
        return -127; // When returns this value, the outcome will be unpredictable.
    }
    *(line_string+i)='\0'; // This is very dangerous. You need to guarantee the length of line_string is long enough!
    if(ch==EOF&&i==0){
        return 1;
    }
    else{
        return 0;
    }
}

//This is a slightly-secure implementation of fgetline()
//Aims to replace the fgetline() if validated.
//Users must make sure that the max_length equals or smaller than the size of the array
//Otherwise, overflow will definately occur.
//return 0: get successed.
//return -127: length invalid.
//return -1: FILE not exist
//return 1: EOF found and read nothing.
//return 127: maxlength 
int fngetline(FILE* file_p, char* line_string, unsigned int max_length){
    int ch='\0';
    int i=0;
    if(max_length<1){
        return -127;
    }
    if(file_p==NULL){
        return -1;
    }
    memset(line_string,'\0',max_length);
    do{
        ch=fgetc(file_p);
        if(ch!=EOF&&ch!='\n'){
            *(line_string+i)=ch;
            i++;
        }
    }while(ch!=EOF&&ch!='\n'&&i!=max_length); // Be careful! This function can only handle lines <= 4096 chars. Extra chars will be ommited
    if(i==max_length){
        return 127; // When returns this value, the outcome will be unpredictable.
    }
    if(ch==EOF&&i==0){
        return 1;
    }
    else{
        return 0;
    }
}

//contain: return 0
//not contain: return 1
int contain_or_not(const char* line, const char* findkey){
    int length_line=strlen(line);
    int length_findkey=strlen(findkey);
    int i,j;
    char* string_temp=(char *)malloc(sizeof(char)*(length_findkey+1));
    for(i=0;i<length_findkey;i++){
        *(string_temp+i)='\0';
    }
    if(length_line<length_findkey){
        free(string_temp);
        return 1;
    }
    for(i=0;i<length_line;i++){
        if(*(line+i)==*(findkey)){
            for(j=0;j<length_findkey;j++){
                *(string_temp+j)=*(line+i+j);
            }
            *(string_temp+length_findkey)='\0';
            if(strcmp(findkey,string_temp)==0){
                free(string_temp);
                return 0;
            }
        }
        else{
            continue;
        }
    }
    free(string_temp);
    return 1;
}

int global_replace(char* filename, char* orig_string, char* new_string){
    if(strcmp(orig_string,new_string)==0){
        return 1;
    }
    if(strlen(orig_string)==0){
        return -1;
    }
    FILE* file_p=fopen(filename, "r");
    FILE* file_p_tmp=NULL;
    if(file_p==NULL){ 
        return -1;
    }
    char single_line[LINE_LENGTH]="";
    char new_line[LINE_LENGTH]="";
    char head=*(orig_string);
    int length_orig=strlen(orig_string);
    char temp_string[LINE_LENGTH]="";
    char filename_temp[FILENAME_LENGTH]="";
    char cmdline[CMDLINE_LENGTH]="";
    int i,j,k,line_length;

    snprintf(filename_temp,511,"%s.tmp",filename);
    file_p_tmp=fopen(filename_temp,"w+");
    if(file_p_tmp==NULL){
        fclose(file_p);
        return -1;
    }
    while(fgetline(file_p,single_line)==0){
        line_length=strlen(single_line);
        if(contain_or_not(single_line,orig_string)!=0||line_length<length_orig){
            fprintf(file_p_tmp,"%s\n",single_line);
            continue;
        }
        i=0;
        j=0;
        reset_string(new_line);
        reset_string(temp_string);
        do{
            if(*(single_line+i)!=head){
                *(new_line+j)=*(single_line+i);
                i++;
                j++;
                continue;
            }
            for(k=0;k<length_orig;k++){
                *(temp_string+k)=*(single_line+i+k);
            }
            if(strcmp(temp_string,orig_string)!=0){
                *(new_line+j)=*(single_line+i);
                i++;
                j++;
                continue;
            }
            else{
                for(k=0;k<strlen(new_string);k++){
                    *(new_line+j+k)=*(new_string+k);
                }
                j=j+k;
                i=i+length_orig;
                continue;
            }
        }while(i<line_length);
        fprintf(file_p_tmp,"%s\n",new_line);
    }
    fclose(file_p);
    fclose(file_p_tmp);
    snprintf(cmdline,2047,"%s %s %s %s",MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
    return 0;
}

int line_replace(char* orig_line, char* new_line, char* orig_string, char* new_string){
    int length=strlen(orig_line);
    int length_orig=strlen(orig_string);
    int length_new=strlen(new_string);
    char* temp_string=(char *)malloc(sizeof(char)*(length_orig+1));
    int i,j;
    int k=0,k2;
    for(i=0;i<length_orig+1;i++){
        *(temp_string+i)='\0';
    }
    if(length_orig==0){
        for(i=0;i<length;i++){
            *(new_line+i)=*(orig_line+i);
        }
        free(temp_string);
        return length;
    }
    reset_string(new_line);
    i=0;
    do{
        if(*(orig_line+i)==*(orig_string)&&i+length_orig<length+1){
            for(j=0;j<length_orig;j++){
                *(temp_string+j)=*(orig_line+i+j);
            }
            if(strcmp(temp_string,orig_string)==0){
                for(j=0;j<length_new;j++){
                    *(new_line+k)=*(new_string+j);
                    k++;
                }
                *(new_line+k)='\0';
                i=i+length_orig;
                for(k2=0;k2<length_orig+1;k2++){
                    *(temp_string+k2)='\0';
                }
                continue;
            }
            else{
                *(new_line+k)=*(orig_line+i);
                i++;
                k++;
                for(k2=0;k2<length_orig+1;k2++){
                    *(temp_string+k2)='\0';
                }
                continue;
            }
        }
        else{
            *(new_line+k)=*(orig_line+i);
            i++;
            k++;
            continue;
        }
    }while(i<length);
    free(temp_string);
    return k;
} 

int find_and_replace(char* filename, char* findkey1, char* findkey2, char* findkey3, char* findkey4, char* findkey5, char* orig_string, char* new_string){
    if(strcmp(orig_string,new_string)==0){
        return -1;
    }
    int replace_count=0;
    FILE* file_p=fopen(filename, "r");
    if(file_p==NULL){
        return -1;
    }
    char filename_temp[FILENAME_LENGTH]="";
    char cmdline[CMDLINE_LENGTH]="";
    char single_line[LINE_LENGTH]="";
    char new_single_line[LINE_LENGTH]="";
    snprintf(filename_temp,511,"%s.tmp",filename);
    int flag1=0,flag2=0,flag3=0,flag4=0,flag5=0,flag6=0;
    FILE* file_temp_p=fopen(filename_temp,"w+");
    if(file_temp_p==NULL){
        fclose(file_p);
        return -1;
    }
    while(fgetline(file_p,single_line)!=1){
        if(strlen(findkey1)!=0){
            flag1=contain_or_not(single_line,findkey1);
        }
        if(strlen(findkey2)!=0){
            flag2=contain_or_not(single_line,findkey2);
        }
        if(strlen(findkey3)!=0){
            flag3=contain_or_not(single_line,findkey3);
        }
        if(strlen(findkey4)!=0){
            flag4=contain_or_not(single_line,findkey4);
        }
        if(strlen(findkey5)!=0){
            flag5=contain_or_not(single_line,findkey5);
        }
        if(flag1!=0||flag2!=0||flag3!=0||flag4!=0||flag5!=0){
            fprintf(file_temp_p,"%s%c",single_line,'\n');
            flag1=0;
            flag2=0;
            flag3=0;
            flag4=0;
            flag5=0;
            continue;
        }
        else{
            flag6=contain_or_not(single_line,orig_string);
            if(flag6!=0){
                fprintf(file_temp_p,"%s%c",single_line,'\n');
            }
            else{
                replace_count++;
                if(strcmp(orig_string,new_string)==0){
                    fprintf(file_temp_p,"%s%c",single_line,'\n');
                    continue;
                }
                line_replace(single_line,new_single_line,orig_string,new_string);
                fprintf(file_temp_p,"%s%c",new_single_line,'\n');
                reset_string(new_single_line);
            }
            flag6=0;
        }
    }
    fprintf(file_temp_p,"%s",single_line);
    fclose(file_p);
    fclose(file_temp_p);
    snprintf(cmdline,2047,"%s %s %s && %s %s %s %s",DELETE_FILE_CMD,filename,SYSTEM_CMD_REDIRECT_NULL,MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
    return replace_count;
}

int find_multi_keys(char* filename, char* findkey1, char* findkey2, char* findkey3, char* findkey4, char* findkey5){
    if(strlen(findkey1)==0&&strlen(findkey2)==0&&strlen(findkey3)==0&&strlen(findkey4)==0&&strlen(findkey5)==0){
        return -1;
    }
    int find_count=0;
    FILE* file_p=fopen(filename, "r");
    if(file_p==NULL){
        return -1;
    }
    char single_line[LINE_LENGTH]="";
    int flag1=0,flag2=0,flag3=0,flag4=0,flag5=0;
    while(fgetline(file_p,single_line)!=1){
        if(strlen(findkey1)!=0){
            flag1=contain_or_not(single_line,findkey1);
        }
        if(strlen(findkey2)!=0){
            flag2=contain_or_not(single_line,findkey2);
        }
        if(strlen(findkey3)!=0){
            flag3=contain_or_not(single_line,findkey3);
        }
        if(strlen(findkey4)!=0){
            flag4=contain_or_not(single_line,findkey4);
        }
        if(strlen(findkey5)!=0){
            flag5=contain_or_not(single_line,findkey5);
        }
        if(flag1!=0||flag2!=0||flag3!=0||flag4!=0||flag5!=0){
            flag1=0;
            flag2=0;
            flag3=0;
            flag4=0;
            flag5=0;
            continue;
        }
        else{
            find_count++;
        }
    }
    fclose(file_p);
    return find_count;
}

int calc_str_num(char* line, char split_ch){
    if(strlen(line)==0){
        return 0;
    }
    int i=0,j=0;
    int str_num=0;
    if(split_ch==' '){
        do{
            if(*(line+i)!=' '&&*(line+i)!='\t'){
                do{
                    j++;
                }while(*(line+i+j)!=' '&&*(line+i+j)!='\t'&&j<strlen(line)-i);
                if(j==(strlen(line)-i)){
                    str_num++;
                    return str_num;
                }
                if(*(line+i+j)==' '||*(line+i+j)=='\t'){
                    str_num++;
                    i=i+j;
                    j=0;
                }
            }
            else{
                i++;
            }
        }while(i<strlen(line));
        return str_num;
    }
    else{
        str_num=0;
        if(*(line)!=split_ch){
            str_num++;
        }
        do{
            if(*(line+i)==split_ch&&*(line+i+1)!=split_ch){
                str_num++;
            }
            i++;
        }while(i<strlen(line));
        return str_num;
    }
}

//return -1: get finished
//return 0: get_successed
int get_seq_string(char* line, char split_ch, int string_seq, char* get_string){
    int total_string_num=calc_str_num(line,split_ch);
    int i=0,j=0;
    int string_seq_current;
    if(string_seq>total_string_num){
        strcpy(get_string,"");
        return -1;
    }
    reset_string(get_string);
    if(split_ch==' '){
        string_seq_current=0;
        if(*(line)!=' '&&*(line)!='\t'){
            string_seq_current++;
        }
        while(string_seq_current<string_seq){
            if(*(line+i)==' '||*(line+i)=='\t'){
                if(*(line+i+1)!=' '&&*(line+i+1)!='\t'){
                    string_seq_current++;
                    i++;
                }
                else{
                    i++;
                }
            }
            else{
                i++;
            }
        }
        for(j=i;j<strlen(line);j++){
            if(*(line+j)==' '||*(line+j)=='\t'){
                break;
            }
            else{
                *(get_string+j-i)=*(line+j);
            }
        }
        return 0;
    }
    else{
        string_seq_current=0;
        if(*(line)!=split_ch){
            string_seq_current++;
        }
        while(string_seq_current<string_seq){
            if(*(line+i)==split_ch){
                if(*(line+i+1)!=split_ch){
                    string_seq_current++;
                    i++;
                }
                else{
                    i++;
                }
            }
            else{
                i++;
            }
        }
        for(j=i;j<strlen(line);j++){
            if(*(line+j)==split_ch){
                break;
            }
            else{
                *(get_string+j-i)=*(line+j);
            }
        }
        return 0;
    }
}

int find_and_get(char* filename, char* findkey_primary1, char* findkey_primary2, char* findkey_primary3, int plus_line_num, char* findkey1, char* findkey2, char* findkey3, char split_ch, int string_seq_num, char* get_string){
    if(strlen(findkey_primary1)==0&&strlen(findkey_primary2)==0&&strlen(findkey_primary3)==0){
        return -1;
    }
    if(strlen(findkey1)==0&&strlen(findkey2)==0&&strlen(findkey3)==0){
        return -1;
    }
    FILE* file_p=fopen(filename, "r");
    if(file_p==NULL){
        return -1;
    }
    char single_line[LINE_LENGTH_EXT]="";
    char get_string_buffer[LINE_LENGTH]="";
    int flag_primary1=0,flag_primary2=0,flag_primary3=0;
    int flag_primary=1;
    int flag1=0,flag2=0,flag3=0;
    int i;
    while(flag_primary!=0&&!feof(file_p)){
        fgetline(file_p,single_line);
        if(strlen(findkey_primary1)!=0){
            flag_primary1=contain_or_not(single_line,findkey_primary1);
        }
        if(strlen(findkey_primary2)!=0){
            flag_primary2=contain_or_not(single_line,findkey_primary2);
        }
        if(strlen(findkey_primary3)!=0){
            flag_primary3=contain_or_not(single_line,findkey_primary3);
        }
        if(flag_primary1==0&&flag_primary2==0&&flag_primary3==0){
            flag_primary=0;
            break;
        }
        else{
            flag_primary=1;
            flag_primary1=0;
            flag_primary2=0;
            flag_primary3=0;
            continue;
        }
    }
    if(feof(file_p)){
        fclose(file_p);
        strcpy(get_string,"");
        return 1;
    }
    i=0;
    while(!feof(file_p)&&i<plus_line_num){
        if(strlen(findkey1)!=0){
            flag1=contain_or_not(single_line,findkey1);
        }
        if(strlen(findkey2)!=0){
            flag2=contain_or_not(single_line,findkey2);
        }
        if(strlen(findkey3)!=0){
            flag3=contain_or_not(single_line,findkey3);
        }
        if(flag1!=0||flag2!=0||flag3!=0){
            flag1=0;
            flag2=0;
            flag3=0;
            i++;
            fgetline(file_p,single_line);
            continue;
        }
        else{
            fclose(file_p);
            get_seq_string(single_line,split_ch,string_seq_num,get_string_buffer);
            strcpy(get_string,get_string_buffer);
            return 0;
        }
    }
    strcpy(get_string,"");
    return 1;
}

//return 0: exists
//return 1: not-exists
int file_exist_or_not(char* filename){
    FILE* file_p=fopen(filename,"r");
    if(file_p==NULL){
        return 1;
    }
    else{
        fclose(file_p);
        return 0;
    }
}

// return <1, empty
// return >1, with contents
int file_empty_or_not(char* filename){
    FILE* file_p=fopen(filename,"r");
    char temp_line[LINE_LENGTH]="";
    int line_num=0;
    if(file_p==NULL){
        return -1;
    }
    else{
        while(fgetline(file_p,temp_line)!=1){
            line_num++;
        }
        fclose(file_p);
        if(strlen(temp_line)>0){
            line_num++;
        }
        return line_num;
    }
}

//return 0: exists
//return non-zero: not exists.
int folder_exist_or_not(char* foldername){
    char filename[FILENAME_LENGTH]="";
    char cmdline[CMDLINE_LENGTH]="";
    snprintf(filename,511,"%s%stestfile.txt",foldername,PATH_SLASH);
    FILE* test_file=fopen(filename,"w+");
    if(test_file==NULL){
        return 1;
    }
    else{
        fclose(test_file);
        snprintf(cmdline,2047,"%s %s %s",DELETE_FILE_CMD,filename,SYSTEM_CMD_REDIRECT_NULL);
        system(cmdline);
        return 0;
    }
}

int password_complexity_check(char* password, const char* special_chars){
    int i,length=strlen(password);
    int uppercase_flag=0;
    int lowercase_flag=0;
    int number_flag=0;
    int special_ch_flag=0;
    char ch_temp[2]="";

    for(i=0;i<length;i++){
        if(*(password+i)=='A'||*(password+i)=='Z'){
            uppercase_flag=1;
        }
        else if(*(password+i)>'A'&&*(password+i)<'Z'){
            uppercase_flag=1;
        }
        else if(*(password+i)=='a'||*(password+i)=='z'){
            lowercase_flag=1;
        }
        else if(*(password+i)>'a'&&*(password+i)<'z'){
            lowercase_flag=1;
        }
        else if(*(password+i)=='0'||*(password+i)=='9'){
            number_flag=1;
        }
        else if(*(password+i)>'0'&&*(password+i)<'9'){
            number_flag=1;
        }
        else{
            *(ch_temp)=*(password+i);
            *(ch_temp+1)='\0';
            if(contain_or_not(special_chars,ch_temp)==0){
                special_ch_flag=1;
            }
            else{
                return 1;
            }
        }
    }
    if((uppercase_flag+lowercase_flag+number_flag+special_ch_flag)<3){
        return 1;
    }
    else{
        return 0;
    }
}

int generate_random_passwd(char* password){
    int i,total_times,rand_num;
    struct timeval current_time;
    char ch_table[72]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~@&(){}[]=";
    char password_temp[PASSWORD_STRING_LENGTH]="";
    unsigned int seed_num;
    for(total_times=0;total_times<10;total_times++){
        for(i=0;i<PASSWORD_LENGTH;i++){
            GETTIMEOFDAY_FUNC(&current_time,NULL);
            seed_num=(unsigned int)(current_time.tv_sec+current_time.tv_usec);
            srand(seed_num);
            rand_num=rand()%72;
            *(password_temp+i)=*(ch_table+rand_num);
            usleep(5000);
        }
//        printf(":::   %s \n",password_temp);
        if(password_complexity_check(password_temp,"~@&(){}[]=")==0){
//            printf(":::   %s \n",password_temp);
            strcpy(password,password_temp);
            return 0;
        }
        reset_string(password_temp);
    }
    return 1;
}

int generate_random_db_passwd(char* password){
    int i,rand_num;
    struct timeval current_time;
    char ch_table[62]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    unsigned int seed_num;
    for(i=0;i<PASSWORD_LENGTH;i++){
        GETTIMEOFDAY_FUNC(&current_time,NULL);
        seed_num=(unsigned int)(current_time.tv_sec+current_time.tv_usec);
        srand(seed_num);
        rand_num=rand()%62;
        *(password+i)=*(ch_table+rand_num);
        usleep(5000);
    }
    return 0;
}

int generate_random_string(char* random_string){
    int i,rand_num;
    struct timeval current_time;
    char ch_table[36]="abcdefghijklmnopqrstuvwxyz0123456789";
    unsigned int seed_num;
    GETTIMEOFDAY_FUNC(&current_time,NULL);
    seed_num=(unsigned int)(current_time.tv_sec+current_time.tv_usec);
    srand(seed_num);
    rand_num=rand()%26;
    *(random_string+0)=*(ch_table+rand_num);
    usleep(5000);
    for(i=1;i<RANDSTR_LENGTH_PLUS-1;i++){
        GETTIMEOFDAY_FUNC(&current_time,NULL);
        seed_num=(unsigned int)(current_time.tv_sec+current_time.tv_usec);
        srand(seed_num);
        rand_num=rand()%36;
        *(random_string+i)=*(ch_table+rand_num);
        usleep(5000);
    }
    *(random_string+RANDSTR_LENGTH_PLUS-1)='\0';
    return 0;  
}

/*
 * CAUTION: THIS IS NOT SUITABLE FOR *NIX 
 */
#ifdef _WIN32
char* getpass_win(char* prompt){
    static char passwd[AKSK_LENGTH];
    char ch='\0';
    int i=0;
    fflush(stdin);
    printf("%s",prompt);
    char BACKSPACE='\b';
    char ENTER='\r';
    while((ch=_getch())!=ENTER&&i!=AKSK_LENGTH-1){
        if(ch!=BACKSPACE&&ch!='\t'&&ch!=' '){
            passwd[i]=ch;
            putchar('*');
            i++;
        }
        else if(ch==BACKSPACE){
            if(i==0){
                continue;
            }
            else{
                printf("\b \b");
                i--;
                passwd[i]='\0';
            }
        }
    }
    passwd[i]='\0';
    printf("\n");
    return passwd;
}
#endif

int insert_lines(char* filename, char* keyword, char* insert_string){
    if(strlen(keyword)==0||strlen(insert_string)==0){
        return -1;
    }
    if(file_exist_or_not(filename)!=0){
        return -3;
    }
    FILE* file_p=fopen(filename,"r");
    FILE* file_p_2=NULL;
    char cmdline[CMDLINE_LENGTH]="";
    char filename_temp[FILENAME_LENGTH]="";
    char single_line[LINE_LENGTH]="";
    int line_num=0;
    int i;
    while(fgetline(file_p,single_line)==0){
        if(contain_or_not(single_line,keyword)==0){
            break;
        }
        else{
            line_num++;
        }
    }
    fseek(file_p,0,SEEK_SET);
    snprintf(filename_temp,511,"%s.tmp",filename);
    file_p_2=fopen(filename_temp,"w+");
    if(file_p_2==NULL){
        fclose(file_p);
        return -1;
    }
    for(i=0;i<line_num;i++){
        fgetline(file_p,single_line);
        fprintf(file_p_2,"%s\n",single_line);
    }
    fprintf(file_p_2,"%s\n",insert_string);
    while(fgetline(file_p,single_line)==0){
        fprintf(file_p_2,"%s\n",single_line);
    }
    fclose(file_p);
    fclose(file_p_2);
    snprintf(cmdline,2047,"%s %s %s %s",MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
    return 0;
}

int local_path_parser(char* path_string, char* path_final){
#ifdef _WIN32
    strcpy(path_final,path_string);
    return 0;
#endif
    int i;
    char path_temp[DIR_LENGTH]="";
    if(strlen(path_string)==0){
        strcpy(path_final,"");
        return 0;
    }
    if(*(path_string+0)=='~'){
        for(i=1;i<strlen(path_string);i++){
            *(path_temp+i-1)=*(path_string+i);
        }
#ifdef __linux__
//        printf("%s       %s    ppppp\n",path_temp,path_final);
        sprintf(path_final,"/home/hpc-now%s",path_temp);
//        printf("%s       %s    ppppp\n",path_temp,path_final);
#elif __APPLE__
        sprintf(path_final,"/Users/hpc-now%s",path_temp);
#else
        strcpy(path_final,path_string);
        return 1;
#endif
    }
    else{
        strcpy(path_final,path_string);
    }
    return 0;
}

int direct_path_check(char* path_string, char* hpc_user, char* real_path){
    char header[256]="";
    char tail[DIR_LENGTH]="";
    int i=0;
    int j;
    while(*(path_string+i)!='/'&&i<strlen(path_string)){
        *(header+i)=*(path_string+i);
        i++;
    }
    for(j=i+1;j<strlen(path_string);j++){
        *(tail+j-i-1)=*(path_string+j);
    }
    if(strcmp(header,"@h")==0){
        if(strcmp(hpc_user,"root")==0){
            sprintf(real_path,"/root/%s",tail);
        }
        else{
            sprintf(real_path,"/home/%s/%s",hpc_user,tail);
        }
        return 0;
    }
    else if(strcmp(header,"@d")==0){
        if(strcmp(hpc_user,"root")==0){
            sprintf(real_path,"/hpc_data/%s",tail);
        }
        else{
            sprintf(real_path,"/hpc_data/%s_data/%s",hpc_user,tail);
        }
        return 0;
    }
    else if(strcmp(header,"@a")==0){
        if(strcmp(hpc_user,"root")==0){
            sprintf(real_path,"/hpc_apps/%s",tail);
        }
        else{
            sprintf(real_path,"/hpc_apps/%s_apps/%s",hpc_user,tail);
        }
        return 0;
    }
    else if(strcmp(header,"@p")==0){
        sprintf(real_path,"/hpc_data/public/%s",tail);
        return 0;
    }
    else if(strcmp(header,"@R")==0){
        if(strcmp(hpc_user,"root")==0||strcmp(hpc_user,"user1")==0){
            sprintf(real_path,"/%s",tail);
        }
        else{
            sprintf(real_path,"/home/%s/%s",hpc_user,tail);
        }
        return 0;
    }
    else if(strcmp(header,"@t")==0){
        sprintf(real_path,"/tmp/%s",tail);
        return 0;
    }
    else{
        strcpy(real_path,path_string);
        return 1;
    }
}

int file_creation_test(char* filename){
    char cmdline[CMDLINE_LENGTH]="";
    if(strlen(filename)==0){
        return -1;
    }
    if(file_exist_or_not(filename)==0){
        return 1;
    }
    FILE* file_p=fopen(filename,"w+");
    if(file_p==NULL){
        return 3;
    }
    fclose(file_p);
    snprintf(cmdline,2047,"%s %s %s",DELETE_FILE_CMD,filename,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
    return 0;
}

int cmd_flg_or_not(char* argv){
    int i;
    for(i=0;i<CMD_FLAG_NUM;i++){
        if(strcmp(argv,command_flags[i])==0){
            return 0;
        }
    }
    return 1;
}

int cmd_key_or_not(char* argv){
    int i;
    for(i=0;i<CMD_KWDS_NUM;i++){
        if(strcmp(argv,command_keywords[i])==0){
            return 0;
        }
    }
    return 1;
}

int cmd_flag_check(int argc, char** argv, char* flag_string){
    int i;
    for(i=2;i<argc;i++){
        if(strcmp(argv[i],flag_string)==0){
            return 0;
        }
    }
    return 1;
}

//return 0: found the keyword
//return 1: not found
//make sure the dest array has 128 width.
int cmd_keyword_check(int argc, char** argv, char* key_word, char* kwd_string){
    int i,j;
    for(i=2;i<argc-1;i++){
        if(strcmp(argv[i],key_word)==0){
            j=i+1;
            if(cmd_flg_or_not(argv[j])!=0&&cmd_key_or_not(argv[j])!=0){
                strncpy(kwd_string,argv[j],127);
                return 0;
            }
            else{
                strcpy(kwd_string,"");
                return 1;
            }
        }
    }
    strcpy(kwd_string,"");
    return 1;
}

//return 0: found the keyword
//return 1: not found
//make sure the dest array has 128 width.
int cmd_keyword_ncheck(int argc, char** argv, char* key_word, char* kwd_string, unsigned int n){
    int i,j;
    for(i=2;i<argc-1;i++){
        if(strcmp(argv[i],key_word)==0){
            j=i+1;
            if(cmd_flg_or_not(argv[j])!=0&&cmd_key_or_not(argv[j])!=0){
                strncpy(kwd_string,argv[j],n);
                return 0;
            }
            else{
                strcpy(kwd_string,"");
                return 1;
            }
        }
    }
    strcpy(kwd_string,"");
    return 1;
}

int include_string_or_not(int cmd_c, char** cmds, char* string){
    int i;
    for(i=0;i<cmd_c;i++){
        if(strcmp(cmds[i],string)==0){
            return 0;
        }
    }
    return 1;
}

int file_cr_clean(char* filename){
#ifndef _WIN32 //For Windows 32 environment, there is no need to clean the \r char.
    if(file_exist_or_not(filename)!=0){
        return -1;
    }
    FILE* file_p=fopen(filename,"r");
    char filename_temp[FILENAME_LENGTH]="";
    int ch;
    char cmdline[CMDLINE_LENGTH]="";
    snprintf(filename_temp,511,"%s.cr.tmp",filename);
    FILE* file_p_tmp=fopen(filename_temp,"w+");
    if(file_p_tmp==NULL){
        fclose(file_p);
        return -1;
    }
    do{
        ch=fgetc(file_p);
        if(ch==EOF){
            break;
        }
        else{
            if(ch!='\r'){
                fputc(ch,file_p_tmp);
            }
            else{
                fputc('\0',file_p_tmp);
            }
        }
    }while(!feof(file_p));
    fclose(file_p);
    fclose(file_p_tmp);
    snprintf(cmdline,2047,"%s %s %s %s",MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
#endif
    return 0;
}

/* This function is risky! It overwrites the original file*/
int file_trunc_by_kwds(char* filename, char* start_key, char* end_key, int overwrite_flag){
    if(file_exist_or_not(filename)!=0){
        return -1;
    }
    if(strlen(start_key)==0&&strlen(end_key)==0){
        return 3;
    }
    if(strcmp(start_key,end_key)==0){
        return 5;
    }
    FILE* file_p=fopen(filename,"r");
    char filename_temp[FILENAME_LENGTH]="";
    char line_buffer[LINE_LENGTH]="";
    int start_flag=0;
    int contain_start_flag;
    int contain_end_flag;
    char cmdline[CMDLINE_LENGTH]="";
    snprintf(filename_temp,511,"%s.trunc.tmp",filename);
    FILE* file_p_tmp=fopen(filename_temp,"w+");
    if(file_p_tmp==NULL){
        fclose(file_p);
        return -1;
    }
    while(!feof(file_p)){
        fgetline(file_p,line_buffer);
        if(strlen(start_key)==0){
            if(contain_or_not(line_buffer,end_key)!=0){
                fprintf(file_p_tmp,"%s\n",line_buffer);
            }
            else{
                break;
            }
        }
        else{
            contain_start_flag=contain_or_not(line_buffer,start_key);
            if(strlen(end_key)!=0){
                contain_end_flag=contain_or_not(line_buffer,end_key);
            }
            else{
                contain_end_flag=-1;
            }
            if(contain_start_flag!=0&&start_flag==0){
                continue;
            }
            else if(contain_start_flag==0&&start_flag==0){
                if(contain_end_flag==0){
                    break;
                }
                fprintf(file_p_tmp,"%s\n",line_buffer);
                start_flag=1;
            }
            else{
                if(contain_end_flag==0){
                    break;
                }
                fprintf(file_p_tmp,"%s\n",line_buffer);
            }
        }
    }
    fclose(file_p);
    fclose(file_p_tmp);
    if(overwrite_flag!=0){
        snprintf(cmdline,2047,"%s %s %s %s",MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
        system(cmdline);
    }
    return 0;
}

//overwrite flag =0, not replace
//overwrite flag !=0, replace.
int delete_lines_by_kwd(char* filename, char* key, int overwrite_flag){
    if(file_exist_or_not(filename)!=0){
        return -1;
    }
    if(strlen(key)==0){
        return -3;
    }
    FILE* file_p=fopen(filename,"r");
    char filename_temp[FILENAME_LENGTH]="";
    char cmdline[CMDLINE_LENGTH]="";
    char line_buffer[LINE_LENGTH]="";
    int getline_flag=0;
    snprintf(filename_temp,511,"%s.del.tmp",filename);
    FILE* file_p_tmp=fopen(filename_temp,"w+");
    if(file_p_tmp==NULL){
        fclose(file_p);
        return -1;
    }
    while(!feof(file_p)){
        getline_flag=fgetline(file_p,line_buffer);
        if(contain_or_not(line_buffer,key)==0){
            continue;
        }
        if(getline_flag==0){
            fprintf(file_p_tmp,"%s\n",line_buffer);
        }
    }
    fclose(file_p);
    fclose(file_p_tmp);
    if(overwrite_flag!=0){
        snprintf(cmdline,2047,"%s %s %s %s",MOVE_FILE_CMD,filename_temp,filename,SYSTEM_CMD_REDIRECT_NULL);
        system(cmdline);
    }
    return 0;
}

//return 0: successfully get md5sum
//return -1: failed to get the md5sum
//THIS FUNCTION IS DEPRECATED.
int get_crypto_key(char* crypto_key_filename, char* md5sum){
    char cmdline[CMDLINE_LENGTH]="";
    FILE* md5_tmp=NULL;
#ifdef _WIN32
    char buffer[256]="";
#endif
#ifdef __APPLE__
    snprintf(cmdline,2047,"md5 '%s' | awk '{print $NF}' > /tmp/md5.txt.tmp",crypto_key_filename);
#elif __linux__
    snprintf(cmdline,2047,"md5sum '%s' | awk '{print $1}' > /tmp/md5.txt.tmp",crypto_key_filename);
#elif _WIN32
    snprintf(cmdline,2047,"certutil -hashfile \"%s\" md5 > c:\\programdata\\md5.txt.tmp",crypto_key_filename);
#endif
    if(system(cmdline)!=0){
        return -1;
    }
#ifdef _WIN32
    md5_tmp=fopen("c:\\programdata\\md5.txt.tmp","r");
#else
    md5_tmp=fopen("/tmp/md5.txt.tmp","r");
#endif
    if(md5_tmp==NULL){
        return -1;
    }
#ifdef _WIN32
    fgetline(md5_tmp,buffer);
#endif
    fgetline(md5_tmp,md5sum);
    fclose(md5_tmp);
#ifdef _WIN32
    snprintf(cmdline,2047,"del /f /q c:\\programdata\\md5.txt.tmp %s",SYSTEM_CMD_REDIRECT_NULL);
#else
    snprintf(cmdline,2047,"rm -rf /tmp/md5.txt.tmp %s",SYSTEM_CMD_REDIRECT_NULL);
#endif
    system(cmdline);
    return 0;
}

//Please *DO* make sure the md5sum_length equals to the md5sum_string array length
//return -3: the given length is invalid
//return 1: get_md5 failed
//return 0: get_md5 succeeded
int get_nmd5sum(char* filename, char md5sum_string[], int md5sum_length){
    if(md5sum_length<33){
        return -3;
    }
    memset(md5sum_string,'\0',md5sum_length);
    int run_flag=now_md5_for_file(filename,md5sum_string,md5sum_length);
    if(run_flag!=0){
        return 1;
    }
    return 0;
}

int password_hash(char* password, char md5_hash[], int md5_length){
    if(strlen(password)==0){
        return -3;
    }
    if(md5_length<33){
        return -5;
    }
    char filename_temp[FILENAME_LENGTH]="";
    char cmdline[CMDLINE_LENGTH]="";
    char string_temp[16]="";
    int run_flag;
    generate_random_string(string_temp);
    FILE* file_p=NULL;
#ifdef _WIN32
    snprintf(filename_temp,511,"c:\\programdata\\%s.tmp",string_temp);
    file_p=fopen(filename_temp,"w+");
    if(file_p==NULL){
        strcpy(md5_hash,"");
        return -1;
    }
    fprintf(file_p,"%s\n",password);
    fclose(file_p);
#else
    snprintf(filename_temp,511,"/tmp/%s.tmp",string_temp);
    file_p=fopen(filename_temp,"w+");
    if(file_p==NULL){
        strcpy(md5_hash,"");
        return -1;
    }
    fprintf(file_p,"%s\r\n",password);
    fclose(file_p);
#endif
    //get_crypto_key(filename_temp,md5_hash);
    run_flag=get_nmd5sum(filename_temp,md5_hash,md5_length);
    snprintf(cmdline,2047,"%s %s %s",DELETE_FILE_CMD,filename_temp,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
    if(run_flag==0){
        return 0;
    }
    else{
        return 1;
    }
}

int windows_path_to_string(char* input_string, char* new_string){
#ifndef _WIN32
    strcpy(new_string,input_string);
    return 0;
#else
    int i,j=0;
    char string_buffer[FILENAME_LENGTH_EXT]=""; //Caution! Stack Overflow may occur.
    char ch_curr,ch_prev,ch_next;
    int length=strlen(input_string);
    for(i=0;i<length;i++){
        ch_curr=*(input_string+i);
        if(i!=0){
            ch_prev=*(input_string+i-1);
        }
        else{
            ch_prev='\0';
        }
        if(i!=length-1){
            ch_next=*(input_string+i+1);
        }
        else{
            ch_next='\0';
        }
        if(j>FILENAME_LENGTH_EXT||j==FILENAME_LENGTH_EXT){
            strcpy(new_string,"");
            return -1;
        }
        if(ch_curr=='\\'&&ch_prev!='\\'&&ch_next!='\\'){
            string_buffer[j]='\\';
            string_buffer[j+1]='\\';
            j=j+2;
        }
        else{
            string_buffer[j]=ch_curr;
            j++;
        }
    }
    strcpy(new_string,string_buffer);
    return 0;
#endif
}

// This function is deprecated!
int base64decode_deprecated(char* encoded_string, char* export_path){
    if(file_creation_test(export_path)!=0){
        return 1;
    }
    char cmdline[CMDLINE_LENGTH_EXT]=""; // Stack Overflow will occur if the encoded string exceeds 4096
    int run_flag;
#ifdef _WIN32
    char filename_temp[FILENAME_LENGTH]="";
    FILE* file_p=NULL;
    snprintf(filename_temp,511,"%s%sbase64_convert.tmp",DESTROYED_DIR,PATH_SLASH);
    file_p=fopen(filename_temp,"w+");
    if(file_p==NULL){
        return 3;
    }
    fprintf(file_p,"%s",encoded_string);
    fclose(file_p);
    snprintf(cmdline,2047,"certutil -decode %s %s %s",filename_temp,export_path,SYSTEM_CMD_REDIRECT_NULL);
    run_flag=system(cmdline);
    snprintf(cmdline,2047,"%s %s %s",DELETE_FILE_CMD,filename_temp,SYSTEM_CMD_REDIRECT_NULL);
    system(cmdline);
#else
    snprintf(cmdline,2047,"echo \"%s\" | base64 -d > %s 2>/dev/null",encoded_string,export_path);
    run_flag=system(cmdline);
#endif
    if(run_flag!=0){
        return 5;
    }
    else{
        return 0;
    }
}

//memory allocated!
char* base64_clear_CRLF(char orig[], int length){
    char* new_string=(char*)malloc(sizeof(char)*length+1);
    memset(new_string,'\0',sizeof(char)*length+1);
    int j=0;
    for(int i=0;i<length;i++){
        if(orig[i]!='\r'&&orig[i]!='\n'){
            *(new_string+j)=orig[i];
            j++;
        }
    }
    return new_string;
}

//Convert a base64 char to an unsigned char
unsigned char get_base64_index(char base64_char){
    if(base64_char=='='){
        return 253;
    }
    else if(base64_char=='+'){
        return 62;
    }
    else if(base64_char=='/'){
        return 63;
    }
    else if(base64_char>64&&base64_char<91){
        return base64_char-65;
    }
    else if(base64_char>47&&base64_char<58){
        return base64_char+4;
    }
    else if(base64_char>96&&base64_char<123){
        return base64_char-71;
    }
    else{
        return 255; //Illegal!
    }
}

//decode a base64 string and print to a file.
int base64decode(char* encoded_string, char* export_path){
    char* encoded_string_new=base64_clear_CRLF(encoded_string,strlen(encoded_string));
    unsigned long length=strlen(encoded_string_new);
    unsigned char* decoded_string=(unsigned char*)malloc(sizeof(unsigned char)*length);
    memset(decoded_string,'\0',length);
    unsigned long group=length>>2;
    unsigned long i,j=0;
    unsigned char ch0,ch1,ch2,ch3;
    //unsigned char char0,char1,char2;
    if(length%4!=0||length<4){
        return -3;
    }
    for(i=0;i<group-1;i++){
        ch0=get_base64_index(encoded_string_new[i*4]);
        ch1=get_base64_index(encoded_string_new[i*4+1]);
        ch2=get_base64_index(encoded_string_new[i*4+2]);
        ch3=get_base64_index(encoded_string_new[i*4+3]);
        if(ch0>63||ch1>63||ch2>63||ch3>63){
            free(encoded_string_new);
            free(decoded_string);
            return 1; //Illegal format
        }
        *(decoded_string+j)=(ch0<<2)|(ch1>>4);
        *(decoded_string+j+1)=(ch1<<4)|(ch2>>2);
        *(decoded_string+j+2)=(ch2<<6)|ch3;
        j+=3;
    }
    ch0=get_base64_index(encoded_string_new[i*4]);
    ch1=get_base64_index(encoded_string_new[i*4+1]);
    ch2=get_base64_index(encoded_string_new[i*4+2]);
    ch3=get_base64_index(encoded_string_new[i*4+3]);
    if(ch0>63||ch1>63||ch2==255||ch3==255){
        free(encoded_string_new);
        free(decoded_string);
        return 1; //Illegal format
    }
    if(ch3!=253&&ch2==253){
        free(encoded_string_new);
        free(decoded_string);
        return 1; //Illegal format
    }
    *(decoded_string+j)=(ch0<<2)|(ch1>>4);
    if(ch3==253){
        if(ch2!=253){
            *(decoded_string+j+1)=(ch1<<4)|(ch2>>2);
        }
    }
    else{
        *(decoded_string+j+1)=(ch1<<4)|(ch2>>2);
        *(decoded_string+j+2)=ch2<<6|ch3;
    }
    free(encoded_string_new);
    //printf(":::::: %s :::::\n",decoded_string);
    FILE* file_p=fopen(export_path,"w+");
    if(file_p==NULL){
        free(decoded_string);
        return -1;
    }
    fprintf(file_p,"%s",decoded_string);
    fclose(file_p);
    free(decoded_string);
    return 0;
}