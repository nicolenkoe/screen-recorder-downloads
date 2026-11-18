#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../windows/samples.h"
int main(void){
 for(int g=0;g<=100;g+=25){
  int16_t mic[]={16000,-16000},system[]={12000,-12000};
  scale_microphone((unsigned char*)mic,2,1,16,0,65);
  scale_microphone((unsigned char*)system,2,1,16,0,g);
  assert(mic[0]==10400&&mic[1]==-10400);
  assert(system[0]==12000*g/100&&system[1]==-12000*g/100);
  float f[]={.8f,-.8f};scale_microphone((unsigned char*)f,2,1,32,1,g);
  assert(fabsf(f[0]-.8f*g/100)<.00001f);
  int32_t i[]={1073741824,-1073741824};scale_microphone((unsigned char*)i,2,1,32,0,g);
  assert(i[0]==(int32_t)(1073741824.0*g/100));
  unsigned char b[]={0,0,0x40,0,0,0xc0};scale_microphone(b,2,1,24,0,g);
  int32_t got=b[0]|b[1]<<8|b[2]<<16;
  assert(got==(int32_t)(4194304.0*g/100));
 }
 puts("PASS: independent microphone/system gain at 0/25/50/75/100 percent; PCM16/24/32 and float32");
}
