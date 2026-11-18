#ifndef RECORDER_SAMPLES_H
#define RECORDER_SAMPLES_H
#include <stdint.h>
#include <string.h>
static int scale_microphone(unsigned char *bytes,unsigned int frames,unsigned int channels,int bits,int floating,int gain){
 double peak=0;unsigned int width=(unsigned int)bits/8;if(!width||!channels)return 0;
 for(uint64_t i=0;i<(uint64_t)frames*channels;i++){unsigned char *p=bytes+i*width;double value=0;
  if(floating&&bits==32){float f;memcpy(&f,p,4);value=f;}else if(!floating&&bits==16){int16_t v;memcpy(&v,p,2);value=v/32768.0;}else if(!floating&&bits==24){int32_t v=(int32_t)((uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16));if(v&0x800000)v-=0x1000000;value=v/8388608.0;}else if(!floating&&bits==32){int32_t v;memcpy(&v,p,4);value=v/2147483648.0;}else continue;
  value*=gain/100.0;if(value>1)value=1;if(value< -1)value=-1;double absValue=value<0?-value:value;if(absValue>peak)peak=absValue;
  if(floating){float f=(float)value;memcpy(p,&f,4);}else if(bits==16){int16_t v=(int16_t)(value>=1?32767:value*32768.0);memcpy(p,&v,2);}else if(bits==24){int32_t v=(int32_t)(value>=1?8388607:value*8388608.0);p[0]=v&255;p[1]=(v>>8)&255;p[2]=(v>>16)&255;}else{int32_t v=(int32_t)(value>=1?2147483647:value*2147483648.0);memcpy(p,&v,4);}
 }return (int)(peak*100.0);
}
#endif
