// MAX7219 commands:
#define CMD_NOOP   0
#define CMD_DIGIT0 1
#define CMD_DIGIT1 2
#define CMD_DIGIT2 3
#define CMD_DIGIT3 4
#define CMD_DIGIT4 5
#define CMD_DIGIT5 6
#define CMD_DIGIT6 7
#define CMD_DIGIT7 8
#define CMD_DECODEMODE  9
#define CMD_INTENSITY   10
#define CMD_SCANLIMIT   11
#define CMD_SHUTDOWN    12
#define CMD_DISPLAYTEST 15

//byte scr[NUM_MAX*8 + 8]; // +8 for scrolled char
struct bitfield
{
    unsigned char bit0:1;
    unsigned char bit1:1;
    unsigned char bit2:1;
    unsigned char bit3:1;
    unsigned char bit4:1;
    unsigned char bit5:1;
    unsigned char bit6:1;
    unsigned char bit7:1;
};

union
{
    struct bitfield bit_data;
    unsigned char byte_data;
}scr1[NUM_MAX * 8 + 8], scr[NUM_MAX * 8 + 8];

void sendCmd(int addr, byte cmd, byte data)
{
  digitalWrite(CS_PIN, LOW);
  for (int i = NUM_MAX-1; i>=0; i--) {
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, i==addr ? cmd : 0);
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, i==addr ? data : 0);
  }
  digitalWrite(CS_PIN, HIGH);
}

void sendCmdAll(byte cmd, byte data)
{
  digitalWrite(CS_PIN, LOW);
  for (int i = NUM_MAX-1; i>=0; i--) {
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, cmd);
    shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, data);
  }
  digitalWrite(CS_PIN, HIGH);
}

//void refresh(int addr) {
  //for (int i = 0; i < 8; i++)
   // sendCmd(addr, i + CMD_DIGIT0, scr[addr * 8 + i]);
//}



////////////////////////////////////////////////////////////////////////////////
void rotate (void)
////////////////////////////////////////////////////////////////////////////////
{
    unsigned char bufAddr = 0;
    unsigned char counter = NUM_MAX;
    
        while(counter--)
    {
        
        scr1[7 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit7 = scr[7 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit6 = scr[6 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit5 = scr[5 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit4 = scr[4 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit3 = scr[3 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit2 = scr[2 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit1 = scr[1 + bufAddr].bit_data.bit7;
        
        scr1[7 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit0;
        scr1[6 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit1;
        scr1[5 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit2;
        scr1[4 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit3;
        scr1[3 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit4;
        scr1[2 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit5;
        scr1[1 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit6;
        scr1[0 + bufAddr].bit_data.bit0 = scr[0 + bufAddr].bit_data.bit7;
        
        bufAddr += 8;
    
    }

    //copy
    //for(counter = 0; counter < NUM_MAX * 8; counter++)
    //{
    //    scr[counter].byte_data = scr1[counter].byte_data;
    //}   
    
}


void refreshAll() {
    
    rotate();
    
    
  for (int c = 0; c < 8; c++) {
    digitalWrite(CS_PIN, LOW);
    for(int i=NUM_MAX-1; i>=0; i--) {
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, CMD_DIGIT0 + c);
      shiftOut(DIN_PIN, CLK_PIN, MSBFIRST, scr1[i * 8 + c].byte_data);
    }
    digitalWrite(CS_PIN, HIGH);
 }
}

void clr()
{
  for (int i = 0; i < NUM_MAX*8; i++) scr[i].byte_data = 0;
}

void scrollLeft()
{
  for(int i=0; i < NUM_MAX*8+7; i++) scr[i].byte_data = scr[i+1].byte_data;
}

void invert()
{
  for (int i = 0; i < NUM_MAX*8; i++) scr[i].byte_data = ~scr[i].byte_data;
}

void initMAX7219()
{
  pinMode(DIN_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  sendCmdAll(CMD_DISPLAYTEST, 0);
  sendCmdAll(CMD_SCANLIMIT, 7);
  sendCmdAll(CMD_DECODEMODE, 0);
  sendCmdAll(CMD_INTENSITY, 1);
  sendCmdAll(CMD_SHUTDOWN, 0);
  clr();
  refreshAll();
}



