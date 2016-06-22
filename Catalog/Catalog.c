#include "Catalog.h"
#include "../Buffer/buffer.h"

FTree THead=NULL;

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

  // alocate space 
  fatAddr = findSpace(); 
  T -> Addr = fatAddr; 
  sh(0xFFFF, fatAddr * 2); 
  addr = TRUEADDR(fatAddr); 
  bufferAddr = getMemoryAddr(addr) + (addr & 0xFFF); 
  pag2 = addr >> 22;  
  PageBuffer[page2t[pag2] * BLOCK_SIZE + ((addr >> 12) & 0x3FF) * 4] |= 0x40; 
  endAddr = bufferAddr + MAX_NAME_LENGTH; 
  // write massage in the buffer 
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

  for(j = 0; j < table -> attrNum; j++)
  {
    endAddr = curAddr + MAX_NAME_LENGTH; 
    for(i = 0; curAddr < endAddr; i++, curAddr++) 
      Buffer[curAddr] = (table -> attributes[j]).name[i]; 

    Buffer[curAddr++] = (table -> attributes[j]).type; 

    for(i = 3; i >= 0; i--, curAddr++) 
      Buffer[curAddr] = ((table -> attributes[j]).size >> i) & 0xFF; 

    Buffer[curAddr++] = (table -> attributes[j]).unique; 

    for(i = 3; i >= 0; i--, curAddr++) 
      Buffer[curAddr] = ((table -> attributes[j]).index >> i) & 0xFF; 

    endAddr = curAddr + MAX_NAME_LENGTH; 
    for(i = 0; curAddr < endAddr; curAddr++, i++) 
      Buffer[curAddr] = (table -> attributes[j]).indexName[i]; 
  }
  return 1; 
}

int RemoveTableFromCatalog(char *name)
{
  FTree T; 
  word curFat, nextFat; 
  T = findFile(name, THead); 
  if(T == NULL) return 0; 
  // free fat space 
  curFat = T -> Addr; 
  nextFat = lh(T -> Addr * 2); 
  sh(0x0000, (T -> Addr) * 2); 
  while(nextFat != 0xFFFF) 
  {
    curFat = nextFat; 
    nextFat = lh(curFat * 2); 
    sh(0x0000, curFat * 2); 
  }
  THead = deleteFile(T, THead); 
  return 1; 
}

int AddIndexToCatalog(char *tableName, int attributeNum, char *indexName)
{
  FTree T; 
  word bufferAddr, curAddr, pag2, ofs, attrNum, attrSize, addr; 
  int i; 
  char aName[MAX_NAME_LENGTH], str[MAX_NAME_LENGTH]; 
  T = findFile(tableName, THead); 
  if(T == NULL) return 0; 
  addr = TRUEADDR(T -> Addr); 
  bufferAddr = getMemoryAddr(addr) + (addr & 0xFFF); 
  pag2 = addr >> 22;  
  PageBuffer[page2t[pag2] * BLOCK_SIZE + ((addr >> 12) & 0x3FF) * 4] |= 0x40; 
  ofs = sizeof(char) * MAX_NAME_LENGTH + sizeof(int) * 4; // before attrNum
  attrSize = sizeof(char) * (MAX_NAME_LENGTH * 2 + 2) + sizeof(int) * 2; 
  curAddr = bufferAddr + ofs; 
  attrNum = (Buffer[curAddr] << 24) | (Buffer[curAddr] << 16) | (Buffer[curAddr] << 8) | Buffer[curAddr];
  if(attributeNum >= attrNum) return 0; 
  curAddr += sizeof(int) + attrSize * attributeNum; 
  for(i = 0; i < MAX_NAME_LENGTH; i++) 
    aName[i] = Buffer[curAddr + i]; 
  aName[i] = 0; 
  ofs = attrSize - sizeof(char) * MAX_NAME_LENGTH; // before indexName 
  curAddr += ofs; 
  for(i = 0; i < MAX_NAME_LENGTH; i++, curAddr++) 
    Buffer[curAddr] = indexName[i]; 
  // create new name for indexes 
  strcpy(str, tableName); 
  strcat(str, "_"); 
  strcat(str, aName); 
  strcat(str, "_"); 
  strcat(str, indexName); 
  T = createFile(str); 
  T -> Addr = findSpace(); 
  sh(0xFFFF, T -> Addr * 2); 
  THead = insertFile(T, THead); 
  return 1; 
}

int RemoveIndexFromCatalog(char *tableName, int attributeNum, char *indexName)
{
  FTree T; 
  word bufferAddr, curAddr, pag2, ofs, attrNum, attrSize, addr; 
  int i; 
  char iName[MAX_NAME_LENGTH], aName[MAX_NAME_LENGTH], str[MAX_NAME_LENGTH]; 
  T = findFile(tableName, THead); 
  if(T == NULL) return 0; 
  addr = TRUEADDR(T -> Addr); 
  bufferAddr = getMemoryAddr(addr) + (addr & 0xFFF); 
  pag2 = addr >> 22;  
  PageBuffer[page2t[pag2] * BLOCK_SIZE + ((addr >> 12) & 0x3FF) * 4] |= 0x40; 
  ofs = sizeof(char) * MAX_NAME_LENGTH + sizeof(int) * 4; // before attrNum
  attrSize = sizeof(char) * (MAX_NAME_LENGTH * 2 + 2) + sizeof(int) * 2; 
  curAddr = bufferAddr + ofs; 
  attrNum = (Buffer[curAddr] << 24) | (Buffer[curAddr] << 16) | (Buffer[curAddr] << 8) | Buffer[curAddr];
  if(attributeNum >= attrNum) return 0; 
  curAddr += sizeof(int) + attrSize * attributeNum; 
  for(i = 0; i < MAX_NAME_LENGTH; i++) 
    aName[i] = Buffer[curAddr + i]; 
  ofs = attrSize - sizeof(char) * MAX_NAME_LENGTH; // before indexName 
  curAddr += ofs; 
  for(i = 0; i < MAX_NAME_LENGTH; i++, curAddr++) 
    iName[i] = Buffer[curAddr]; 
  if(strcmp(iName, indexName) != 0) return 0; 
  // delete the respect file 
  strcpy(str, tableName); 
  strcat(str, "_"); 
  strcat(str, aName); 
  strcat(str, "_"); 
  strcat(str, indexName); 
  T = findFile(str, THead); 
  if(T == NULL) return 0; 
  deleteFile(T, THead); 
  return 1; 
}

word findSpace() 
{
  static word address = 0; 
  half fat; 
  word cnt = 0; 
  address += 1; 
  if(address == 0x10000) address = 1; 
  fat = lh(address * 2); 
  while(fat != 0) 
  {
    address += 1; 
    if(address == 0x10000)
    {
      address = 1; 
      cnt++;
      if(cnt >=2) 
        return 0xFFFFFFFF; 
    }
    fat = lh(address * 2); 
  }
  return address; 
}

// using Tree opertion 
FTree createFile(char *str)
{
  FTree T; 
  T = (FTree) malloc(sizeof(struct FileTree)); 
  if(T == NULL) {printf("Memory allocation failed!\n"); return NULL;} 
  strcpy(T -> name, str); 
  T -> Addr = 0xFFFFFFFF; 
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
      T -> Addr = Temp -> Addr; 
      T -> Right = deleteFile(Temp, T -> Right); 
    }
    else if(T -> Left == NULL) 
    {
      Temp = T; 
      T = T -> Right; 
      sh(0x0000, 2 * Temp -> Addr); 
      free(Temp); 
    }
    else 
    {
      Temp = T; 
      T = T -> Left; 
      sh(0x0000, 2 * Temp -> Addr); 
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
