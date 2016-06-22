int __CY__DEBUG=0;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MiniSQL.h"
#include "Record/Record.h"
#include "BPlusTree/BPlusTree.h"
int TRUEFLAG=1;
int FLAG_INPUT_FINISH=1;
const int F=-1;
#define safe(function) do {if(function==F) goto False;if(TRUEFLAG==F) goto False;} while(0)
#define safe2() do {if(TRUEFLAG==F) goto False;} while(0)
#define safe3(function,str) do {strcpy(error_message,str);if(function==NULL) goto False;if(TRUEFLAG==F) goto False;strcpy(error_message,"");} while(0)
#define print printf
//If a function returns F, which means error occurs, all steps should be stop and return.

//Global Variables:
char t[9][999];//temporary string memory, the buffer
char error_message[99];

void ErrorSyntax(const char* s);

//Basic Functions:
#define e(str1,str2) (strcmp(str1,str2)==0) //string equal,e("a","a"): 1, e("a","b"): 0
#define in(str1,str2) (strstr(str2,str1)!=NULL)//str1 in str2
char* w(char* s,int id) { //w:getword, s:"create table classroom...", id:buffer id, return: "create", s=t[id]:"table classroom..."
    int i=0,l=strlen(s);
    char* p=t[id];
    while(s[i]==' '&&i<l) i++;//skip first blanks
    for(; i<l; i++) {
        if (s[i]==' ') break;
        *p++ = s[i];
    }*p=0;
    while(s[i]==' '&&i<l) i++;//skip second blanks
    strcpy(s,s+i);
    return t[id];
}/*TEST:
    char test[]="  a  b   c";
    puts(w(test,0));          //a
    puts(test);            //b   c
*/
char* i_get_dh(char* s,int id){//s:"a,b,c",id:buffer id, return=t[id]:"a",s:"b,c"
    char temp[9999];
    if (!in(",",s)) {
        strcpy(t[id],s);
        s[0]=0;
    }else{
        int i=0,l=strlen(s);char* p=t[id];
        for(i=0;i<l;i++){
            if(s[i]==',') break;
            *p++=s[i];
        }*p=0;
        strcpy(temp,s+i+1);
        strcpy(s,temp);
    }
    return t[id];
}/*TEST:
    char test[]="a,bbb,c";
    puts(i_get_dh(test,0));   //a
    puts(test);            //bbb,c
    char test2[]="abbbc";
    puts(i_get_dh(test2,0));  //abbbc
    puts(test2);              //
*/
char* i_get_and(char* s,int id){//s:"a<=10 and b>2 and c='hello'",id:buffer id, return=t[id]:"a < 1",s:"b>2 and c='hello'"
    char temp[9999];
    int i=0,l=strlen(s);char* p=t[id];
    if (!in("and",s)) {
        for(i=0;i<l;i++){
            if(s[i]=='<'||s[i]=='='||s[i]=='>'||s[i]=='!'){
                *p++=' ';
                *p++=s[i];
                if(s[i+1]=='=') *p++=s[++i];
                *p++=' ';
            }else{
                *p++=s[i];
            }
        }*p=0;
        s[0]=0;
    }else{
        for(i=0;i<l-3;i++){
            if(strncmp(s+i," and",4)==0) break;
            if(s[i]=='<'||s[i]=='='||s[i]=='>'||s[i]=='!'){
                *p++=' ';
                *p++=s[i];
                if(s[i+1]=='=') *p++=s[++i];
                *p++=' ';
            }else{
                *p++=s[i];
            }
        }*p=0;
        strcpy(temp,s+i+5);
        strcpy(s,temp);
    }
    return t[id];
}/*TEST:
    char test[]="a<=100 and b>2 and c='hello'";
    puts(i_get_and(test,0));   //a <= 100
    puts(test);             //b>2 and c='hello'
    puts(i_get_and(test,0));   //b > 2
    puts(test);             //c='hello'
    puts(i_get_and(test,0));   //c = 'hello'
    puts(test);             //
*/
char* i_get_kh(char* s,int id){//Get the content in the (), s:"(xh char(10), ..) ;",id: buffer id, return=t[id]:"xh char(10), .."
     /*Debug*/if(__CY__DEBUG) {printf("Debug_i_get_kh:%s\n",s);}
char temp[9999];
    char* p=t[id];int i=0,l=strlen(s), count=1;
    if(s[0]!='(') {return NULL;}
    while(count>0&&i<l){
        i++;
        if(s[i]=='(') count++;
        else if(s[i]==')') count--;
        if(count>0) *p++=s[i];
    }*p=0;
    if(s[i]!=')')  {return NULL;}
    strcpy(temp,s+i+1);
    strcpy(s,temp);
         /*Debug*/if(__CY__DEBUG) {printf("Debug_i_get_kh_After:%s\n",t[id]);}
    return t[id];
}
/*TEST:
    char test[]="(abc)";
    puts(i_get_kh(test,0));
*/
char* trim(char* src){
    char temp[9999];int i=0,j=strlen(src),k;
    strcpy(temp,src);
    for(k=0;k<j;k++) if(temp[k]=='\n'||temp[k]=='\r'||temp[k]=='\t') temp[k]=' ';
    j=strlen(temp)-1;
    while(temp[i]==' ') i++;
    /*while(j>0) {
        if(temp[j--]==';') { temp[j+1]=0; break; }
    }*/
    j=strlen(temp)-1;
    while(temp[j]==' '||temp[j]==';') j--;
    temp[j+1]=0;
    strcpy(src,temp+i);
    return src;
}/*TEST:
    char test[]="  a, bb  ";
    puts(trim(test));
*/


//API
struct AttributeRecord GetAttribute(Table table,char* name,int* attrIndex){//name: Attribute name, return: the struct AttributeRecord
    int i;struct AttributeRecord x;
    for(i=0;i<table->attrNum;i++)
        if(e(name,table->attributes[i].name)) break;
    if(i==table->attrNum){sprintf(error_message,"column \"%s\" not found in the table",name);TRUEFLAG=F;return x;}
    *attrIndex=i;
    return table->attributes[i];
}
struct TableRecord GetTable(char* table_name){
    static char buffer_name[256]={0}; static struct TableRecord buffer_t;
    if(0) if(strcmp(table_name,buffer_name)==0) return buffer_t;
 
     char fileName[256];struct TableRecord t;
     sprintf(fileName,  "%s_record.db", table_name);
     Table table = (Table)ReadBlock(fileName,0,sizeof(struct TableRecord));
     if (NULL == table)
     {
         strcpy(error_message, "Table not found!");
         TRUEFLAG = F;
         return t;
     }
     memcpy(&t,table,sizeof(struct TableRecord));
     strcpy(buffer_name,table_name);
     buffer_t = t;
     free(table);
     return t;
 }
    /*
struct TableRecord __DEBUG__table;//this is for my module test, delete it when catalog finished
struct TableRecord GetTable(char* table_name){//this is for my module test, delete it when catalog finished
    struct TableRecord x;x.name[0]=0;
    if(!e(table_name,__DEBUG__table.name)){
        sprintf(error_message,"Table %s not found in database!",table_name);
        TRUEFLAG=F;
        return x;
    }else return __DEBUG__table;
}

int CreateTable(Table x){
    int i; struct AttributeRecord a;
    print("Function: CreateTable\n");
    print("TABLE:\n name:%s\n attrNum:%d\n primaryKey:%d\n recordSize:%d\n recordNum:%d\n recordsPerBlock:%d\n",x->name,x->attrNum,x->primaryKey,x->recordSize,x->recordNum,x->recordsPerBlock);
    for(i=0;i<x->attrNum;i++){
        a=x->attributes[i];
        //print(" Name:%s\n Type:%d\n Size:%d\n Unique:%d\n Index:%d\n",a.name,a.type,a.size,(int)a.unique,a.index);
    }
    __DEBUG__table=*x;
    return 0;
}
int RemoveTable(Table x){
    print("Function: RemoveTable\n");
    print("TableName:%s\n",x->name);
    return 0;
}
off_t InsertTuple(Table table,char* tuple){
    int i,p=0;
    print("TableName:%s\nData:\n",table->name);
    for(i=0;i<table->attrNum;i++){
        print("  %s:",table->attributes[i].name);
        if(table->attributes[i].type==intType) {print("%d\n",*(int*)(tuple+p));p+=4;}
        else if(table->attributes[i].type==floatType) {print("%f\n",*(float*)(tuple+p));p+=4;}
        else if(table->attributes[i].type==stringType) {print("%s\n",tuple+p);p+=table->attributes[i].size;}
    }
    return 0;
}
int SearchTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter, int *projection){
    int i;
    print("\nFUNCTION: SearchTuples\nTableName:%s\n",table->name);
    print("IntFilter:\n");
    while(intFilter!=NULL) {
        print("  Index %d, Cond %d, Value %d\n",intFilter->attrIndex,intFilter->cond,intFilter->src);
        intFilter=intFilter->next;
    }
    print("FloatFilter:\n");
    while(floatFilter!=NULL) {
        print("  Index %d, Cond %d, Value %f\n",floatFilter->attrIndex,floatFilter->cond,floatFilter->src);
        floatFilter=floatFilter->next;
    }
    print("StrFilter:\n");
    while(strFilter!=NULL) {
        print("  Index %d, Cond %d, Value %s\n",strFilter->attrIndex,strFilter->cond,strFilter->src);
        strFilter=strFilter->next;
    }
    print("Projection:\n  ");
    for(i=0;i<table->attrNum;i++) print("%d,",projection[i]);print("\n");
    return 0;
}
int DeleteTuples(Table table, IntFilter intFilter, FloatFilter floatFilter, StrFilter strFilter){
    int i;
    print("\nFUNCTION: DeleteTuples\nTableName:%s\n",table->name);
    print("IntFilter:\n");
    while(intFilter!=NULL) {
        print("  Index %d, Cond %d, Value %d\n",intFilter->attrIndex,intFilter->cond,intFilter->src);
        intFilter=intFilter->next;
    }
    print("FloatFilter:\n");
    while(floatFilter!=NULL) {
        print("  Index %d, Cond %d, Value %f\n",floatFilter->attrIndex,floatFilter->cond,floatFilter->src);
        floatFilter=floatFilter->next;
    }
    print("StrFilter:\n");
    while(strFilter!=NULL) {
        print("  Index %d, Cond %d, Value %s\n",strFilter->attrIndex,strFilter->cond,strFilter->src);
        strFilter=strFilter->next;
    }
    return 0;
}
*/

//Interpreter Function, i_...
void ErrorSyntax(const char* s){
    TRUEFLAG=F;
    sprintf(error_message,"Syntax doesn't obey:%s",s);
}

struct AttributeRecord i_create_table_attribute(char* s){//s:"xh char(10) unique"
    struct AttributeRecord x;int size;
    w(s,6);
    strcpy(x.name,t[6]);
    w(s,4);
    if(e(t[4],"integer")||e(t[4],"int")){
        x.type=intType;
#ifdef DEBUG
    //printf("t[0]: %s\nt[1]: %s\nt[2]: %s\nt[3]: %s\nt[4]: \"%s\"\n", t[0], t[1], t[2], t[3], t[4]);
#endif
        size=4;
    }else if(e(t[4],"float")){
        x.type=floatType;
        size=4;
    }else if(sscanf(t[4],"char(%d)",&size)==1){
        x.type=stringType;
        size+=1;
    }else {
        ErrorSyntax("int/float/char()");
        size=255;
    }
    x.size=size;
    x.unique=(char)in("unique",s);
    x.index=-1;
    return x;
}/*TEST:
    char test[]="xh int unique";
    struct AttributeRecord a=i_create_table_attribute(test);safe2();
    printf(" Name:%s\n Type:%d\n Size:%d\n Unique:%d\n Index:%d\n",a.name,a.type,a.size,(int)a.unique,a.index);
*/


int i_create_table(char* table_name,char* s){//table_name:"student", s:"xh char(10) unique, name char(20)"
    struct TableRecord ta;int i=0,j=0;
    strcpy(ta.name,table_name);
    ta.primaryKey=-1;
    for(i=0;i<MAX_ATTRIBUTE_NUM;i++) ta.attributes[i].name[0]=0;
    i=0;
    while(!e(s,"")){
        i_get_dh(s,5);//s.split(',')
        if(!in("primary key",t[5])) {ta.attributes[i++]=i_create_table_attribute(t[5]);safe2();}//normal one
        else {
            trim(t[5]);
            if(strstr(t[5],"primary key")==t[5]){//"primary key (xh)"
                //printf("[DEBUG]In primary key:%s",t[5]);
                char key_name[MAX_NAME_LENGTH]={0};
                if(in("(",t[5])) sscanf(trim(t[5]+strlen("primary key")),"(%[^)])",key_name);
                else strcpy(key_name,trim(t[4]+strlen("primary key")));
                for(j=0;j<MAX_ATTRIBUTE_NUM&&ta.attributes[j].name[0];j++){
                    if(e(ta.attributes[j].name,key_name)){
                        ta.primaryKey=j;
                        break;
                    }
                }
                if(!ta.attributes[j].name[0]) {
                    strcpy(error_message,"primary key not found in your attributes");
                    TRUEFLAG=F;
                    return F;
                }
            }else{
                ta.attributes[i++]=i_create_table_attribute(t[5]);safe2();
                ta.primaryKey=i-1;
            }
        }
    }
    ta.attrNum=i;
    ta.recordSize=0;
    for(j=0;j<ta.attrNum;j++){
        ta.recordSize+=ta.attributes[j].size;
    }
    ta.recordNum=0;
    ta.recordsPerBlock=BLOCK_SIZE/ta.recordSize;
    return CreateTable(&ta);
False:
    return F;
}
int i_create_index(char* table_name,char* s){
    struct TableRecord t=GetTable(table_name);safe2();
    int status=CreateIndex(&t,s);safe2();
    return status;
False:
    return F;
}
    

int i_create_table_get_kh(char* s,int id){//Get the content in the (), s:"(xh char(10), ..) ;",id: buffer id, return 0 or F
    char* p=t[id];int i=0,l=strlen(s), count=1;
    if(s[0]!='(') {ErrorSyntax("create table table_name (");return F;}
    while(count>0&&i<l){
        i++;
        if(s[i]=='(') count++;
        else if(s[i]==')') count--;
        if(count>0) *p++=s[i];
    }
    if(s[i]!=')')  {ErrorSyntax("create table table_name (...)");return F;}
    strcpy(s,s+i+1);
    return 0;
}

int i_create(char* s) {
    w(s,1);
    if(e(t[1],"table")) {
        w(s,2);//get the table name
        safe(i_create_table_get_kh(s,3));
        safe(i_create_table(t[2],t[3]));
    }
    else if(e(t[1],"index")) {
        w(s,2);w(s,3); //w(s,4);w(s,5);safe3(i_get_kh(t[5],6),"expecting (column_name)");
        int i,len=strlen(s);char* p=t[4];
        for(i=0;i<len;i++){ 
            if(s[i]!='(') *p++=s[i];
            else{
                *p=0;
                safe3(i_get_kh(s+i,6),"expecting (column_name)");
                break;
            }
        }
        trim(t[4]);trim(t[6]);
        //printf(" INDEX NAME:%s\n TABLE NAME:%s\n COLUMN NAME:%s\n",t[2],t[4],t[6]);
        i_create_index(t[4],t[6]);
    }
    else {ErrorSyntax("create table/index");return  F;}
    return 0;
False:
    return F;
}

int i_drop(char* s){
    w(s,1);
    if(e(t[1],"table")) {
        w(s,2);//get the table name
        struct TableRecord x;
        x=GetTable(s);
        RemoveTable(&x);safe2();
    }
    else if(e(t[1],"index")) {
        w(s,2);
        printf("INDEX %s Droped!\n",t[2]);
    }
    else {ErrorSyntax("drop table/index");return  F;}
    return 0;
False:
    return F;
}

int i_insert(char* s){
    int i;
    w(s,1);
    struct TableRecord table=GetTable(t[1]);safe2();
    if(strstr(s,"values")!=s) { ErrorSyntax("insert into table_name values"); return F;}
    i_get_kh(trim(s+strlen("values")),2);
    char* data = (char*)malloc(table.recordSize);char *p=data;
    memset(data,0,table.recordSize);
     /*Debug*/if(__CY__DEBUG) {printf("Debug:%s",t[2]);}
    for(i=0;i<table.attrNum;i++){
        i_get_dh(t[2],3);
        if(table.attributes[i].type==intType){
            int x;
            sscanf(t[3],"%d",&x);
            memcpy(p,&x,4);
            p+=4;
        }else if(table.attributes[i].type==floatType){
            float x;
            sscanf(t[3],"%f",&x);
            memcpy(p,&x,4);
            p+=4;
        }else if(table.attributes[i].type==stringType){
            trim(t[3]);
            if((t[3][0]=='\"'||t[3][0]=='\'')&&(t[3][strlen(t[3])-1]=='\"'||t[3][strlen(t[3])-1]=='\'')){
                t[3][strlen(t[3])-1]=0;
                if(strlen(t[3]+1)>=table.attributes[i].size) {
                    sprintf(error_message,"\"%s\" size too large, max length:%d",t[3]+1,table.attributes[i].size-1);
                    return F;
                }
                memcpy(p,t[3]+1,strlen(t[3]+1));
                p+=table.attributes[i].size;
            }else{
                sprintf(error_message,"%s, missing \"\"",t[3]);
                return F;
            }
        }
    }
    InsertTuple(&table,data);
    return 0;
False:
    return F;
}/*TEST:
//In the main:
    char x[]="create table student (xh char(10) unique primary key,id int,name char(20),major char(30),GPA float);";
    interpreter(x);//this is preparing for call insert
    char y[]="insert into student values (\"3140105754\",1,\"Chen Yuan\",\"Biology\",3.55)";
    safe(interpreter(y));
//In this function:
    printf("%f",*(float*)(data+71-4));
*/

int i_select(char* s){
    int projection[MAX_ATTRIBUTE_NUM],i,j,attrIndex;enum CmpCond cond;
    struct IntFilterType memory_i[MAX_ATTRIBUTE_NUM];IntFilter pi=NULL,pi_now=NULL;int I_F=-1;//the chain memory from array, pi->chain first node, I_F chain now node index
    struct FloatFilterType memory_f[MAX_ATTRIBUTE_NUM];FloatFilter pf=NULL,pf_now=NULL;int F_F=-1;
    struct StrFilterType memory_s[MAX_ATTRIBUTE_NUM];StrFilter ps=NULL,ps_now=NULL;int S_F=-1;

    for(i=0;i<MAX_ATTRIBUTE_NUM;i++) projection[i]=-1;
    w(s,1);
    if(!e("from",w(s,2))){ErrorSyntax("select ... from");return F;}
    w(s,2);
    struct TableRecord table=GetTable(t[2]);safe2();
    if(e("*",t[1])){
        for(i=0;i<table.attrNum;i++) projection[i]=i;
    }else{
        i=0;
        while(!e(i_get_dh(t[1],3),"")){
            for(j=0;j<table.attrNum;j++)
                if(e(t[3],table.attributes[j].name)) break;
            if(j==table.attrNum){sprintf(error_message,"column \"%s\" not found in the table",t[3]);TRUEFLAG=F;return F;}
            projection[i++]=j;
        }
    }



    if(!e(s,"")){
        if(!e(w(s,4),"where")) {ErrorSyntax("select ... from ... where");return F;}
        while(!e(i_get_and(s,4),"")){
            w(t[4],5);w(t[4],6);strcpy(t[7],t[4]);//w(t[4],7);if(!e(t[4],""))  {sprintf(error_message,"Unexpected %s",t[4]);return F;}
            struct AttributeRecord a=GetAttribute(&table,t[5],&attrIndex);safe2();
            if(e(t[6],"=")) cond=EQUAL;
            else if(e(t[6],"!=")) cond=NOTEQUAL;
            else if(e(t[6],">")) cond=LARGER;
            else if(e(t[6],"<")) cond=SMALLER;
            else if(e(t[6],">=")) cond=LARGERE;
            else if(e(t[6],"<=")) cond=SMALLERE;
            else {sprintf(error_message,"Unexpected %s",t[6]);return F;}
            if(a.type==intType) {
                I_F++;
                if(!sscanf(t[7],"%d",&memory_i[I_F].src)) {sprintf(error_message,"Unexpected %s, missing integer",t[7]);return F;}
                memory_i[I_F].attrIndex=attrIndex;
                memory_i[I_F].cond=cond;
                memory_i[I_F].next=NULL;
                if(pi==NULL) {
                    pi=pi_now=&memory_i[I_F];
                }else{
                    pi_now->next=&memory_i[I_F];
                    pi_now=&memory_i[I_F];
                }
            }else if(a.type==floatType) {
                F_F++;
                if(!sscanf(t[7],"%f",&memory_f[F_F].src)) {sprintf(error_message,"Unexpected %s, missing float",t[7]);return F;}
                memory_f[F_F].attrIndex=attrIndex;
                memory_f[F_F].cond=cond;
                memory_f[F_F].next=NULL;
                if(pf==NULL) {
                    pf=pf_now=&memory_f[F_F];
                }else{
                    pf_now->next=&memory_f[F_F];
                    pf_now=&memory_f[F_F];
                }
            }else if(a.type==stringType) {
                S_F++;
                if((t[7][0]=='\"'&&t[7][strlen(t[7])-1]=='\"')||(t[7][0]=='\''&&t[7][strlen(t[7])-1]=='\'')){
                    t[7][strlen(t[7])-1]=0;
                    strcpy(memory_s[S_F].src,t[7]+1);
                    memory_s[S_F].attrIndex=attrIndex;
                    memory_s[S_F].cond=cond;
                    memory_s[S_F].next=NULL;
                    if(ps==NULL) {
                        ps=ps_now=&memory_s[S_F];
                    }else{
                        ps_now->next=&memory_s[S_F];
                        ps_now=&memory_s[S_F];
                    }
                }else {sprintf(error_message,"Unexpected %s, missing \"...\"",t[7]);return F;}
            }
        }
    }
    return SearchTuples(&table,pi,pf,ps,projection);
False:
    return F;
}

int i_delete(char* s){
    int attrIndex;enum CmpCond cond;
    struct IntFilterType memory_i[MAX_ATTRIBUTE_NUM];IntFilter pi=NULL,pi_now=NULL;int I_F=-1;//the chain memory from array, pi->chain first node, I_F chain now node index
    struct FloatFilterType memory_f[MAX_ATTRIBUTE_NUM];FloatFilter pf=NULL,pf_now=NULL;int F_F=-1;
    struct StrFilterType memory_s[MAX_ATTRIBUTE_NUM];StrFilter ps=NULL,ps_now=NULL;int S_F=-1;
    if(!e("from",w(s,1))){ErrorSyntax("delete from");return F;}
    w(s,2);
    struct TableRecord table=GetTable(t[2]);safe2();
    if(!e(s,"")){
        if(!e(w(s,4),"where")) {ErrorSyntax("delete from ... where");return F;}
        while(!e(i_get_and(s,4),"")){
            w(t[4],5);w(t[4],6);strcpy(t[7],t[4]);//w(t[4],7);if(!e(t[4],""))  {sprintf(error_message,"Unexpected %s",t[4]);return F;}
            struct AttributeRecord a=GetAttribute(&table,t[5],&attrIndex);safe2();
            if(e(t[6],"=")) cond=EQUAL;
            else if(e(t[6],"!=")) cond=NOTEQUAL;
            else if(e(t[6],">")) cond=LARGER;
            else if(e(t[6],"<")) cond=SMALLER;
            else if(e(t[6],">=")) cond=LARGERE;
            else if(e(t[6],"<=")) cond=SMALLERE;
            else {sprintf(error_message,"Unexpected %s",t[6]);return F;}
            if(a.type==intType) {
                I_F++;
                if(!sscanf(t[7],"%d",&memory_i[I_F].src)) {sprintf(error_message,"Unexpected %s, missing integer",t[7]);return F;}
                memory_i[I_F].attrIndex=attrIndex;
                memory_i[I_F].cond=cond;
                memory_i[I_F].next=NULL;
                if(pi==NULL) {
                    pi=pi_now=&memory_i[I_F];
                }else{
                    pi_now->next=&memory_i[I_F];
                    pi_now=&memory_i[I_F];
                }
            }else if(a.type==floatType) {
                F_F++;
                if(!sscanf(t[7],"%f",&memory_f[F_F].src)) {sprintf(error_message,"Unexpected %s, missing float",t[7]);return F;}
                memory_f[F_F].attrIndex=attrIndex;
                memory_f[F_F].cond=cond;
                memory_f[F_F].next=NULL;
                if(pf==NULL) {
                    pf=pf_now=&memory_f[F_F];
                }else{
                    pf_now->next=&memory_f[F_F];
                    pf_now=&memory_f[F_F];
                }
            }else if(a.type==stringType) {
                S_F++;
                if((t[7][0]=='\"'&&t[7][strlen(t[7])-1]=='\"')||(t[7][0]=='\''&&t[7][strlen(t[7])-1]=='\'')){
                    t[7][strlen(t[7])-1]=0;
                    strcpy(memory_s[S_F].src,t[7]+1);
                    memory_s[S_F].attrIndex=attrIndex;
                    memory_s[S_F].cond=cond;
                    memory_s[S_F].next=NULL;
                    if(ps==NULL) {
                        ps=ps_now=&memory_s[S_F];
                    }else{
                        ps_now->next=&memory_s[S_F];
                        ps_now=&memory_s[S_F];
                    }
                }else {sprintf(error_message,"Unexpected %s, missing \"...\"",t[7]);return F;}
            }
        }
    }
    return DeleteTuples(&table,pi,pf,ps);
False:
    return F;
}

void print_error(){
    fprintf(stderr,"Error: %s\n",error_message);
};

int interpreter_more(char *s,char* history);


int i_exec(char* filename){
#ifdef DEBUG
    /*Debug:*/  //printf("[trim_before]%s\n",filename);
#endif
    trim(filename);
#ifdef DEBUG
        /*Debug:*/ //printf("[trim_after]%s\n",filename);
#endif
    FILE* fp=fopen(filename,"r");
    char line[9999],history[9999];
    if(fp==NULL) {sprintf(error_message,"File \"%s\" Not Found!",filename);TRUEFLAG=F;return F;}
    while(fgets(line,9999,fp)!=NULL) {
        interpreter_more(line,history);
    }
    return 0;
}

void i_quit(){
    puts("Goodbye~");
    exit(0);
}

int interpreter(char* command){
    char s[9999];
    strcpy(s,command);
    //if (in("name100",s)) __CY__DEBUG=1;
    if(strlen(s)>0&&s[strlen(s)-1]==';') s[strlen(s)-1]=0;
    w(s,0);
    if(e(t[0],"create")) safe(i_create(s));
    else if(e(t[0],"drop")) safe(i_drop(s));
    else if(e(t[0],"insert")&&e(w(s,0),"into")) safe(i_insert(s));
    else if(e(t[0],"select")) safe(i_select(s));
    else if(e(t[0],"delete")) safe(i_delete(s));
    else if(e(t[0],"exec")) safe(i_exec(s));
    else if(e(t[0],"execfile")) safe(i_exec(s));
    else if(e(t[0],"quit")||e(t[0],"exit")) i_quit();
    else {ErrorSyntax("create/drop/select/insert/delete");goto False;}
    return 0;
False:
#ifdef DEBUG
    //printf("t[0]: \"%s\"\n", t[0]);
    //printf("result: %d\n", e(t[0], "create"));
#endif
    printf("[Fail]%s\n",command);
    print_error();
    error_message[0]=0;
    TRUEFLAG=1;
    return F;
}

int interpreter_more(char *s,char* history){
    int ret;
    if(e("quit",s)||e("exit",s)) i_quit();
    strcat(history,s);
    if(in(";",s)) {
        ret=interpreter(trim(history));
        FLAG_INPUT_FINISH=1;
        history[0]=0;
        return ret;
    }else{
        FLAG_INPUT_FINISH=0;
        return 0;
    }
}
