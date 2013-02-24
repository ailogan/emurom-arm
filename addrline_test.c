#include <stdio.h>

//The low address is in bits 2-7
int low_incr(int addr_low){
  int addr = addr_low >> 2;
  
  addr++;
  
  if(addr > 0x3F){
    addr = 0;
  }

  return (addr << 2);
}

//The high address is in bits 0-5
int high_incr(int addr_high){
  addr_high++;

  if(addr_high > 0x3F){
    addr_high = 0;
  }

  return addr_high;
}

int main(){
  int addr_low  = 0;
  int addr_high = 0;

  int addr = 0;

  do{
    addr = (addr_high << 6) | (addr_low >> 2);
     printf("high: 0x%02x\tlow: 0x%02x\tcombined: 0x%04x\n", addr_high, addr_low, addr);

    addr_low  = low_incr(addr_low);
    
    if(addr_low == 0){
      addr_high = high_incr(addr_high);
    }
    
  }while((addr_low != 0) || (addr_high != 0));
}

