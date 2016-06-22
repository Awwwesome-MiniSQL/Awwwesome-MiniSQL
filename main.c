#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "MiniSQL.h"
#include "Record/Record.h"
#include "BPlusTree/BPlusTree.h"
#include "BPlusTree/BPlusTreeInt.h"
#include "BPlusTree/BPlusTreeFloat.h"
#include "BPlusTree/BPlusTreeStr.h"
#include "Catalog/Catalog.h"
#include "interpreter.c"

char storage_command[9999];
char FLAG_RECORD_INFO = 1;
typedef int (*CmdProcFunc)(char*,char*);
typedef struct{
    char         *pszCmd;
    CmdProcFunc  fpCmd;
}CMD_PROC;

//命令表项宏，用于简化书写
#define CMD_ENTRY(cmdStr, func)     {cmdStr, func}
#define CMD_ENTRY_END               {NULL,   NULL}

//命令表
static CMD_PROC gCmdMap[] = {
    CMD_ENTRY("create",       interpreter_more),
    CMD_ENTRY("select",        interpreter_more),
    CMD_ENTRY("delete",        interpreter_more),
    CMD_ENTRY("insert",        interpreter_more),
    CMD_ENTRY_END
};
#define CMD_MAP_NUM     (sizeof(gCmdMap)/sizeof(CMD_PROC)) - 1/*End*/

//返回gCmdMap中的CmdStr列(必须为只读字符串)，以供CmdGenerator使用
static char *GetCmdByIndex(unsigned int dwCmdIndex)
{
    if(dwCmdIndex >=  CMD_MAP_NUM)
        return NULL;
    return gCmdMap[dwCmdIndex].pszCmd;
}




//退出交互式调测器的命令(不区分大小写)
static const char *pszQuitCmd[] = {"Quit"};
static const unsigned char ucQuitCmdNum = sizeof(pszQuitCmd) / sizeof(pszQuitCmd[0]);
static int IsUserQuitCmd(char *pszCmd)
{
    unsigned char ucQuitCmdIdx = 0;
    for(; ucQuitCmdIdx < ucQuitCmdNum; ucQuitCmdIdx++)
    {
        if(!strcasecmp(pszCmd, pszQuitCmd[ucQuitCmdIdx]))
            return 1;
    }

    return 0;
}


//剔除字符串首尾的空白字符(含空格)
static char *StripWhite(char *pszOrig)
{
    if(NULL == pszOrig)
        return "NUL";

    char *pszStripHead = pszOrig;
    while(isspace(*pszStripHead))
        pszStripHead++;

    if('\0' == *pszStripHead)
        return pszStripHead;

    char *pszStripTail = pszStripHead + strlen(pszStripHead) - 1;
    while(pszStripTail > pszStripHead && isspace(*pszStripTail))
        pszStripTail--;
    *(++pszStripTail) = '\0';

    return pszStripHead;
}

static char *pszLineRead = NULL;  //终端输入字符串
static char *pszStripLine = NULL; //剔除前端空格的输入字符串
char *ReadCmdLine()
{
     //若已分配命令行缓冲区，则将其释放
    if(pszLineRead)
    {
        free(pszLineRead);
        pszLineRead = NULL;
    }
    //读取用户输入的命令行
    if(FLAG_INPUT_FINISH) pszLineRead = readline("MiniSQL>");
    else  pszLineRead = readline("      ->");

    //剔除命令行首尾的空白字符。若剔除后的命令不为空，则存入历史列表
    pszStripLine = StripWhite(pszLineRead);
    if(pszStripLine && *pszStripLine)
        add_history(pszStripLine);

    return pszStripLine;
}

static char *CmdGenerator(const char *pszText, int dwState)
{
    static int dwListIdx = 0, dwTextLen = 0;
    if(!dwState)
    {
        dwListIdx = 0;
        dwTextLen = strlen(pszText);
    }

    //当输入字符串与命令列表中某命令部分匹配时，返回该命令字符串
    const char *pszName = NULL;
    while((pszName = GetCmdByIndex(dwListIdx)))
    {
        dwListIdx++;

        if(!strncmp (pszName, pszText, dwTextLen))
            return strdup(pszName);
    }

    return NULL;
}

static char **CmdCompletion (const char *pszText, int dwStart, int dwEnd)
{
    //rl_attempted_completion_over = 1;
    char **pMatches = NULL;
    if(0 == dwStart)
        pMatches = rl_completion_matches(pszText, CmdGenerator);

    return pMatches;
}

//初始化Tab键能补齐的Command函数
static void InitReadLine(void)
{
    rl_attempted_completion_function = CmdCompletion;
}

int main(void){
    printf("      Welcome to MiniSQL Command!\n");
    printf("      Author: 谢嘉豪, 张扬光, 陈源\n");
    printf("      Press 'quit' or 'exit' to quit.\n\n");
    InitReadLine();
    while(1){
        char *pszCmdLine = ReadCmdLine();
        if(IsUserQuitCmd(pszCmdLine))
        {
            free(pszLineRead);
            break;
        }
        if(in("exec",pszCmdLine)) {
            FLAG_RECORD_INFO=0;
            interpreter_more(pszCmdLine,storage_command);
            FLAG_RECORD_INFO=1;
        }else interpreter_more(pszCmdLine,storage_command);
    }

    return 0;
}
