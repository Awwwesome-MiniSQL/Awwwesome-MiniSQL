#include "buffer.h" 

TRecord *rHead, *rTail; 
int MemUse[TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM]; 

byte *page2v; 
byte *page2t; 
byte *PageBuffer; 
byte *Buffer; 

int log2(long num)
{
  
  if(num < 2) 
    return 0; 
  return log2(num / 2) + 1; 
}

int log2c(long num)
{
  int temp; 
  temp = log2(num); 
  temp = (num % 2 == 0) ? temp : temp + 1; 
  return temp; 
}

void createDataBase(void) 
{
  FILE *fp; 
  byte zero = 0; 
  fp = fopen(MDISK, "wb"); 
  if(fp == NULL) return 0; 
  fwrite(&zero, sizeof(zero), PAGE_SIZE, fp); 
  fclose(fp); 
  fp = fopen(DISK, "wb"); 
  if(fp == NULL) return 0; 
  fwrite(&zero, sizeof(zero), TOTAL_NUM * TOTAL_BASE, fp); 
  fclose(fp); 
  return 0; 
}

void initMemory()
{
  int i; 

  if(createDataBase() == 0) exit(0); 

  rHead = (TRecord *)malloc(sizeof(struct TimeRecord)); 
  if(rHead == NULL) {printf("Memory allocation failed!\n"); return;} 
  rHead -> Next = NULL; 
  rTail = rHead; 

  page2v = (byte *) malloc(sizeof(byte) * PAGE2_NUM); 
  if(page2v == NULL) {printf("Memory allocation failed!\n"); return;} 
  page2t = (byte *) malloc(sizeof(byte) * PAGE2_NUM); 
  if(page2t == NULL) {printf("Memory allocation failed!\n"); return;} 
  PageBuffer = (byte *) malloc(sizeof(byte) * BUFFER_PAGE_SIZE); 
  if(PageBuffer == NULL) {printf("Memory allocation failed!\n"); return;} 
  Buffer = (byte *) malloc(sizeof(byte) * DATA_SIZE); 
  if(Buffer == NULL) {printf("Memory allocation failed!\n"); return;} 
  for(i = 0; i < PAGE2_NUM; i++) 
    page2v[i] = 0; 
  return ; 
  for(i = 0; i < PAGE2_NUM; i++) 
    page2t[i] = 0; 
  return ; 
  for(i = 0; i < BUFFER_PAGE_SIZE; i++) 
    PageBuffer[i] = 0; 
  return ; 
  for(i = 0; i < DATA_SIZE; i++) 
    Buffer[i] = 0; 

  return ; 
}

void freeMemory()
{
  TRecord *P, *Temp; 
  free(page2v); 
  free(page2t); 
  free(PageBuffer); 
  free(Buffer); 
  for(P = rHead; P != NULL; P = Temp) 
  {
    Temp = P -> Next; 
    free(P); 
  }
  return ; 
}

void useMemory(word index)
{
  TRecord *P, *Pt; 
  P = (TRecord *)malloc(sizeof(struct TimeRecord)); 
  if(P == NULL) {printf("Memory allocation failed!\n"); return ;} 
  P -> index = index; 
  P -> time = time(0); 
  P -> Next = NULL; 
  rTail -> Next = P; 
  rTail = P; 
  MemUse[index]++; 
  return ; 
}

int LRU()
{
  TRecord *P, *Temp; 
  word rcnt = rTail -> time; 
  int i, Min = 0x7FFF, MinIndex = -1; 
  for(P = rHead; P -> Next != NULL; P = P -> Next) 
  {
    if(rcnt - P -> Next -> time >= MAXINT)
    {
      MemUse[P -> Next -> index]--; 
      Temp = P -> Next; 
      P -> Next = P -> Next -> Next; 
      free(Temp); 
    }
  }
  rTail = P; 
  rcnt = TOTAL_BASE / BLOCK_SIZE * TOTAL_NUM; 
  for(i = 0; i < rcnt; i++) 
  {
    if(MemUse[i] < Min)
    {
      MinIndex = i; 
      Min = MemUse[i]; 
    }
  }
  return MinIndex; 
}

word getMemoryAddr(word addr)
{
  int v2, v1, p1, p2, d1, i; 
  word pag2, pag1, tag1; 
  word temp, p, t; 
  FILE *fp; 
  pag2 = addr >> (2 * BLOCK_BIT); 
  v2 = page2v[pag2]; 
  if(v2 == 0) 
  {
    fp = fopen(MDISK, "rb+"); 
    if(fp == NULL) {printf("open file failed!\n"); return 0xFFFFFFFF;} 
    p2 =  rand() % (BUFFER_PAGE_SIZE / BLOCK_SIZE); 
    for(i = 0; i < PAGE2_NUM; i++) 
      if(page2v[i] != 0 && page2t[i] == p2) 
        break; 
    if(i < PAGE2_NUM)
    {
      fseek(fp, i * BLOCK_SIZE, SEEK_SET); 
      fwrite(&PageBuffer[p2 * BLOCK_SIZE], sizeof(byte), BLOCK_SIZE, fp); 
      page2v[i] = 0; 
    }
    fseek(fp, pag2 * BLOCK_SIZE, SEEK_SET); 
    fread(&PageBuffer[p2 * BLOCK_SIZE], sizeof(byte), BLOCK_SIZE, fp); 
    page2v[pag2] = 1; 
    page2t[pag2] = p2; 
    fclose(fp); 
  }
  temp = page2t[pag2] * BLOCK_SIZE; 
  pag1 = PageBuffer[temp] << 24 | PageBuffer[temp + 1] << 16 | PageBuffer[temp + 2] << 8 | PageBuffer[temp + 3]; 
  v1 = pag1 >> 31; 
  d1 = (pag1 >> 30) & 0x1; 
  tag1 = (pag1 >> 10) & 0xFFFFF; 
  if(v1 == 0) 
  {
    fp = fopen(DISK, "rb+"); 
    if(fp == NULL) {printf("open file failed!\n"); return 0xFFFFFFFF;} 
    p1 = LRU(); 
      for(i = 0; i < BUFFER_PAGE_SIZE; i += 4) 
        if(PageBuffer[i] & 0x80)
        {
          p = (PageBuffer[i+2] & 0x3) << 8 | (PageBuffer[i+3]); 
          if(p == p1) 
            break; 
        }
      if(i < BUFFER_PAGE_SIZE) 
      {
        if(d1 != 0) 
        {
          fseek(fp, ((PageBuffer[i] & 0x3F) << 14 | (PageBuffer[i+1] << 6) | (PageBuffer[i+2] >> 2)) * BLOCK_SIZE, SEEK_SET); 
          fwrite(&Buffer[p1 * BLOCK_SIZE], sizeof(byte), BLOCK_SIZE, fp); 
        }
        PageBuffer[i] &= 0x7F; 
      }
    fseek(fp, tag1 * BLOCK_SIZE, SEEK_SET); 
    fread(&Buffer[p1 * BLOCK_SIZE], sizeof(byte), BLOCK_SIZE, fp); 
    pag1 = ((pag1 >> 10) << 10) | p1; 
    pag1 |= 0x80000000; 
    PageBuffer[temp] = (pag1 >> 24) & 0xFF; 
    PageBuffer[temp+1] = (pag1 >> 16) & 0xFF; 
    PageBuffer[temp+2] = (pag1 >> 8) & 0xFF; 
    PageBuffer[temp+3] = pag1 & 0xFF; 
    fclose(fp); 
  }
  useMemory(tag1); 
  return (pag1 & 0x3FF) * BLOCK_SIZE; 
}

// big-endian 
byte lb(word addr)
{
  return Buffer[getMemoryAddr(addr) + addr & 0xFFF]; 
}

void sb(word addr, byte data)
{
  word buffAddr; 
  word pag2; 
  buffAddr = getMemoryAddr(addr) + addr & 0xFFF; 
  pag2 = addr >> (2 * BLOCK_BIT);  
  PageBuffer[page2t[pag2]] |= 0x40; 
  Buffer[buffAddr] = data; 
  return ; 
}

half lh(word addr)
{
  return (lb(addr) << 8) | (lb(addr + 1)); 
}

void sh(word addr, half data)
{
  sb(addr, data >> 8); 
  sb(addr + 1, data & 0xFF); 
  return ; 
}

word lw(word addr)
{
  return (lb(addr) << 24) | (lb(addr + 1) << 16) | (lb(addr + 2) << 8) | (lb(addr + 3)); 
}

void sw(word addr, word data)
{
  sb(addr, data >> 24); 
  sb(addr + 1, (data >> 16) & 0xFF); 
  sb(addr + 2, (data >> 8) & 0xFF); 
  sb(addr + 3, data & 0xFF); 
  return ; 
}

word getFat(char *name, word num)
{
  FTree T; 
  word fatAddr, nextAddr, i; 
  T = findFile(name, THead); 
  if(T == NULL) return 0x00000000; 
  fatAddr = T -> Addr; 
  nextAddr = lh(fatAddr * 2); 
  i = 0; 
  while(nextAddr != 0xFFFF && i < num)
  {
    fatAddr = nextAddr; 
    nextAddr = lh(fatAddr * 2); 
    i++; 
  }
  return fatAddr; 
}

word ReadBlock(char *name, word num)
{
  word fatAddr; 
  fatAddr = getFat(name, num); 
  if(fatAddr == 0x0000) return 0xFFFFFFFF; 
  return getMemoryAddr(TRUEADDR(fatAddr)); 
}

int WriteBlock(char *name, word num, byte *block)
{
  word bufferAddr, addr, fatAddr, pag2; 
  FTree T; 
  word i; 
  fatAddr = getFat(name, num); 
  if(fatAddr == 0x0) return 0; 
  bufferAddr = getMemoryAddr(TRUEADDR(fatAddr)); 
  for(i = 0; i < BLOCK_SIZE; i++, bufferAddr++) 
    Buffer[bufferAddr] = block[i]; 
  addr = TRUEADDR(fatAddr); 
  pag2 = addr >> (2 * BLOCK_BIT);  
  PageBuffer[page2t[pag2]] |= 0x40; 
  return 1; 
}
