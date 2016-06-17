#include <stdio.h>
#include "../Buffer/buffer.h"
#include "Catalog.h"

FTree THead;

int AddTableToCatalog(Table table)
{
  FTree T;
  word fatAddr, bufferAddr, curAddr, endAddr, addr;
  word pag2;
  int i, j;

  T = findFile(table -> name, THead);
  if(T != NULL)
    return 0;
  T = createFile(table -> name);
  THead = insertFile(T, THead);

  fatAddr = findSpace();
  T -> Addr = fatAddr;
  sh(0xFFFF, fatAddr);
  addr = TRUEADDR(fatAddr);
  bufferAddr = getMemoryAddr(addr);
  pag2 = addr >> (2 * BLOCK_BIT);
  PageBuffer[page2t[pag2]] |= 0x40;
  endAddr = bufferAddr + MAX_NAME_LENGTH;
  for(i = 0, curAddr = bufferAddr; curAddr < endAddr; curAddr++, i++)
    Buffer[curAddr] = table -> name[i];

  for(i = 3; i >= 0; i--, curAddr++)
    Buffer[curAddr] = (table -> recordSize >> i) & 0xFF;

  for(i = 3; i >= 0; i--, curAddr++)
    Buffer[curAddr] = (table -> recordNum >> i) & 0xFF;

  for(i = 3; i >= 0; i--, curAddr++)
    Buffer[curAddr] = (table -> recordsPerBlock >> i) & 0xFF;

  for(i = 3; i >= 0; i--, curAddr++)
    Buffer[curAddr] = (table -> primaryKey >> i) & 0xFF;

  for(i = 3; i >= 0; i--, curAddr++)
    Buffer[curAddr] = (table -> attrNum >> i) & 0xFF;

  // @TODO table -> attributes
  for(j = 0; j < attrNum; j++)
  {
    endAddr = curAddr + MAX_NAME_LENGTH;
    for(i = 0; curAddr < endAddr; i++, curAddr++)
      Buffer[curAddr] = (table -> attributes).name[i];

    Buffer[curAddr++] = (table -> attributes).type;

    for(i = 3; i >= 0; i--, curAddr++)
      Buffer[curAddr] = ((table -> attributes).size >> i) & 0xFF;

    Buffer[curAddr++] = (table -> attributes).unique;

    for(i = 3; i >= 0; i--, curAddr++)
      Buffer[curAddr] = ((table -> attributes).index >> i) & 0xFF;

    endAddr = curAddr + MAX_NAME_LENGTH;
    for(i = 0; curAddr < endAddr; curAddr++, i++)
      Buffer[curAddr] = (table -> attributes).indexName[i];
  }
  return 1;
}

int RemoveTableFromCatalog(char *name)
{
  FTree T;
  T = fineFile(name, THead);
  if(T == NULL) return 0;
  sh(0x0000, T -> Addr);
  THead = deleteFile(T, THead);
  return 1;
}

int AddIndexToCatalog(char *tableName, int attributeNum, char *indexName)
{
  FTree T;
  word bufferAddr, curAddr, pag2, ofs, attrNum, attrSize;
  int num;
  int i;
  T = findFile(tableName, THead);
  if(T == NULL) return 0;
  addr = TRUEADDR(T -> Addr);
  bufferAddr = getMemoryAddr(addr);
  pag2 = addr >> (2 * BLOCK_BIT);
  PageBuffer[page2t[pag2]] |= 0x40;
  ofs = sizeof(char) * MAX_NAME_LENGTH + sizeof(int) * 4; // before attrNum
  attrSize = sizeof(char) * (MAX_NAME_LENGTH * 2 + 2) + sizeof(int) * 2;
  curAddr = bufferAddr + ofs;
  attrNum = (Buffer[curAddr] << 24) | (Buffer[curAddr] << 16) | (Buffer[curAddr] << 8) | Buffer[curAddr];
  if(attributeNum >= attrNum) return 0;
  curAddr += sizeof(int) + attrSize * attributeNum;
  ofs = attrSize - sizeof(char) * MAX_NAME_LENGTH; // before indexName
  curAddr += ofs;
  for(i = 0; i < MAX_NAME_LENGTH; i++, curAddr++)
    Buffer[curAddr] = indexName[i];
  return 1;
}

int RemoveIndexFromCatalog(char *name)
{
  FTree T;
  T = fineFile(name, THead);
  if(T == NULL) return 0;
  sh(0x0000, T -> Addr);
  THead = deleteFile(T, THead);
  return 1;
}

word findSpace()
{
  static word address = -2;
  half fat;
  word cnt = 0;
  address += 2;
  if(address == 0x20000) address = 0;
  fat = lh(address);
  while(fat != 0)
  {
    address += 2;
    if(address == 0x20000)
    {
      address = 0;
      cnt++;
      if(cnt >=2)
        return 0xFFFFFFFF;
    }
    fat = lh(address);
  }
  return address;
}

FTree createFile(char *str)
{
  FTree T;
  T = (FTree) malloc(sizeof(struct FileTree));
  if(T == NULL) {printf("Memory allocation failed!\n"); return NULL;}
  strcpy(T -> name, str);
  T -> CtlgAddr = 0xFFFFFFFF;
  T -> IndxAddr = 0xFFFFFFFF;
  T -> RcrdAddr = 0xFFFFFFFF;
  T -> Left = T -> Right = NULL;
  return T;
}

FTree findFile(char *str, FTree T)
{
  if(T == NULL) return NULL;
  if(strcmp(str, T -> name) < 0)
    return findFile(str, T -> Left);
  else if(strcmp(str, T -> name) == 0)
    return T;
  else if(strcmp(str, T -> name) > 0)
    return findFile(str, T -> Right);
  else
    return NULL;
}

FTree insertFile(FTree P, FTree T)
{
  if(P == NULL)
    return T;
  if(T == NULL)
    T = P;
  else if(strcmp(P -> name, T -> name) < 0)
    T -> Left = insertFile(P, T -> Left);
  else if(strcmp(P -> name, T -> name) == 0)
    free(P);
  else
    T -> Right = insertFile(P, T -> Right);
  return T;
}

FTree findMin(FTree T)
{
  if(T == NULL)
    return NULL;
  while(T -> Left != NULL)
    T = T -> Left;
  return T;
}

FTree deleteFile(FTree P, FTree T)
{
  FTree Temp;
  if(T == NULL || P == NULL)
    return T;
  if(strcmp(P -> name, T -> name) < 0)
    T -> Left = deleteFile(P, T -> Left);
  else if(strcmp(P -> name, T -> name) > 0)
    T -> Right = deleteFile(P, T -> Right);
  else
  {
    if(T -> Right != NULL && T -> Left != NULL)
    {
      Temp = findMin(T -> Right);
      strcpy(T -> name, Temp -> name);
      T -> CtlgAddr = Temp -> CtlgAddr;
      T -> IndxAddr = Temp -> IndxAddr;
      T -> RcrdAddr = Temp -> RcrdAddr;
      T -> Right = deleteFile(Temp, T -> Right);
    }
    else if(T -> Left == NULL)
    {
      Temp = T;
      T = T -> Right;
      free(Temp);
    }
    else
    {
      Temp = T;
      T = T -> Left;
      free(Temp);
    }
  }
  return T;
}

void freeFile(FTree T)
{
  if(T == NULL) return;
  freeFile(T -> Left);
  freeFile(T -> Right);
  free(T);
  return ;
}
