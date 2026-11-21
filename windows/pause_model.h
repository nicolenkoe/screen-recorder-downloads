#ifndef RECORDER_PAUSE_MODEL_H
#define RECORDER_PAUSE_MODEL_H
#include "edit_model.h"
#include <stdlib.h>
#include <string.h>
/* FFmpeg freezedetect timestamps are normalized with setpts=PTS-STARTPTS. */
static int pause_read(FILE *f,double duration,double retain,EditRange *found,double *removed){
 char line[4096];int count=0;double start=-1;*removed=0;
 while(fgets(line,sizeof(line),f)){
  char *p=strstr(line,"freeze_start: ");if(p){start=strtod(p+14,NULL);continue;}
  p=strstr(line,"freeze_end: ");if(p&&start>=0){double end=fmin(duration,strtod(p+12,NULL)),from=start+retain;
   if(end>from+.001){if(count==EDIT_MAX_RANGES)return -1;found[count++]=(EditRange){from,end};*removed+=end-from;}start=-1;
  }
 }
 if(start>=0&&duration>start+retain+.001){if(count==EDIT_MAX_RANGES)return -1;found[count++]=(EditRange){start+retain,duration};*removed+=duration-start-retain;}
 return count;
}
/* Only silence interiors may be cut. Preserve 250 ms on both sides of sound. */
static int pause_protect(FILE *log,const EditRange *cuts,int count,double duration,EditRange *out){
 char line[4096];double start=-1;int n=0;
 while(fgets(line,sizeof(line),log)){
  char *p=strstr(line,"silence_start: ");if(p){start=fmax(0,strtod(p+15,NULL));continue;}
  p=strstr(line,"silence_end: ");if(p&&start>=0){double end=fmin(duration,strtod(p+13,NULL));
   double a=start>0?start+.25:0,b=end<duration-.001?end-.25:duration;
   for(int i=0;i<count;i++){double from=fmax(a,cuts[i].start),to=fmin(b,cuts[i].end);if(to>from+.001){if(n==EDIT_MAX_RANGES)return -1;out[n++]=(EditRange){from,to};}}start=-1;
  }
 }
 if(start>=0){double a=start>0?start+.25:0;for(int i=0;i<count;i++){double from=fmax(a,cuts[i].start),to=cuts[i].end;if(to>from+.001){if(n==EDIT_MAX_RANGES)return -1;out[n++]=(EditRange){from,to};}}}
 return n;
}
#endif
