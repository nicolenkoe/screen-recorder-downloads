#ifndef RECORDER_EDIT_MODEL_H
#define RECORDER_EDIT_MODEL_H
#include <stdio.h>
#include <wchar.h>
#include <math.h>
#define EDIT_MAX_RANGES 64
typedef struct { double start,end; } EditRange;
static double edit_clamp(double x,double duration){if(!(x>=0))return 0;return x>duration?duration:x;}
static void edit_time(wchar_t *out,size_t size,double seconds,int milliseconds){
 if(!(seconds>=0)||seconds>3153600000.)seconds=0;
 unsigned long long n=(unsigned long long)(seconds*1000.+.5),s=n/1000;
 if(milliseconds){
  if(s>=3600)swprintf(out,size,L"%02llu:%02llu:%02llu.%03llu",s/3600,s/60%60,s%60,n%1000);
  else swprintf(out,size,L"%02llu:%02llu.%03llu",s/60,s%60,n%1000);
 }else{
  s=(unsigned long long)seconds;
  if(s>=3600)swprintf(out,size,L"%02llu:%02llu:%02llu",s/3600,s/60%60,s%60);
  else swprintf(out,size,L"%02llu:%02llu",s/60,s%60);
 }
}
static double edit_tick_step(double duration,int slots){
 static const double steps[]={1,2,5,10,15,30,60,120,300,600,900,1800,3600,7200,14400,28800,86400};
 double target=duration/(slots>0?slots:1);
 for(unsigned int i=0;i<sizeof(steps)/sizeof(*steps);i++)if(steps[i]>=target)return steps[i];
 return ceil(target/86400.)*86400.;
}
/* Returns sorted, merged KEEP intervals for either edit mode, or -1 for invalid input. */
static int edit_plan(const EditRange *input,int count,double duration,int remove,EditRange *out){
 if(!(duration>0)||!isfinite(duration)||count<1||count>EDIT_MAX_RANGES)return -1;
 EditRange sorted[EDIT_MAX_RANGES],merged[EDIT_MAX_RANGES];
 for(int i=0;i<count;i++){
  if(!isfinite(input[i].start)||!isfinite(input[i].end)||input[i].start<0||input[i].end<=input[i].start||input[i].end>duration+.000001)return -1;
  sorted[i]=input[i];if(sorted[i].end>duration)sorted[i].end=duration;
  for(int j=i;j>0&&sorted[j].start<sorted[j-1].start;j--){EditRange t=sorted[j];sorted[j]=sorted[j-1];sorted[j-1]=t;}
 }
 int n=0;
 for(int i=0;i<count;i++){if(n&&sorted[i].start<=merged[n-1].end+.000001){if(sorted[i].end>merged[n-1].end)merged[n-1].end=sorted[i].end;}else merged[n++]=sorted[i];}
 if(!remove){for(int i=0;i<n;i++)out[i]=merged[i];return n;}
 int k=0;double at=0;
 for(int i=0;i<n;i++){if(merged[i].start>at+.000001)out[k++]=(EditRange){at,merged[i].start};at=merged[i].end;}
 if(at<duration-.000001)out[k++]=(EditRange){at,duration};
 return k;
}
/* An ASCII filter script avoids command-line length limits and unsafe filename interpolation. */
static int edit_write_graph(FILE *f,const EditRange *ranges,int count,int audio){
 if(!f||count<1||count>EDIT_MAX_RANGES+1)return 0;
 fprintf(f,"[0:v:0]setpts=PTS-STARTPTS,split=%d",count);for(int i=0;i<count;i++)fprintf(f,"[v%d]",i);fprintf(f,";\n");
 if(audio){fprintf(f,"[0:a:0]asetpts=PTS-STARTPTS,asplit=%d",count);for(int i=0;i<count;i++)fprintf(f,"[a%d]",i);fprintf(f,";\n");}
 for(int i=0;i<count;i++){
  fprintf(f,"[v%d]trim=start=%.6f:end=%.6f,setpts=PTS-STARTPTS[pv%d];\n",i,ranges[i].start,ranges[i].end,i);
  if(audio)fprintf(f,"[a%d]atrim=start=%.6f:end=%.6f,asetpts=PTS-STARTPTS[pa%d];\n",i,ranges[i].start,ranges[i].end,i);
 }
 for(int i=0;i<count;i++){fprintf(f,"[pv%d]",i);if(audio)fprintf(f,"[pa%d]",i);}
 fprintf(f,"concat=n=%d:v=1:a=%d[vout]",count,audio?1:0);if(audio)fprintf(f,"[aout]");fprintf(f,"\n");
 return !ferror(f);
}
#endif
