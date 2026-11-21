package ru.screenrecorder.app;
final class AudioMix {
 static short sample(short microphone,short system,float micGain,float systemGain,boolean micOn,boolean systemOn){
  float value=(micOn?microphone*micGain:0)+(systemOn?system*systemGain:0);
  value*=micOn&&systemOn?.7f:1;return (short)Math.max(-32768,Math.min(32767,Math.round(value)));
 }
}
