#include <assert.h>
#include <string.h>
#include "../windows/pause_model.h"
int main(int argc,char **argv){
 EditRange ranges[EDIT_MAX_RANGES];double removed;
 if(argc>1){FILE *f=fopen(argv[1],"rb");assert(f);int n=pause_read(f,17,1,ranges,&removed);fclose(f);assert(n==2);assert(fabs(ranges[0].start-1)<.1&&fabs(ranges[0].end-5)<.1);assert(fabs(ranges[1].start-8)<.1&&ranges[1].end==17);assert(fabs(removed-13)<.1);puts("PASS: actual 5s and 10s freezes retain 1s each; moving section preserved");return 0;}
 FILE *audio=tmpfile();assert(audio);fputs("silence_start: 0\nsilence_end: 2\nsilence_start: 4\nsilence_end: 8\nsilence_start: 9\n",audio);rewind(audio);EditRange cut={1,10};int safe=pause_protect(audio,&cut,1,10,ranges);assert(safe==3);assert(ranges[0].start==1&&ranges[0].end==1.75);assert(ranges[1].start==4.25&&ranges[1].end==7.75);assert(ranges[2].start==9.25&&ranges[2].end==10);fclose(audio);
 FILE *f=tmpfile();assert(f);fputs("freeze_start: 0\nfreeze_end: 5\nfreeze_start: 7\n",f);rewind(f);assert(pause_read(f,17,1,ranges,&removed)==2);assert(removed==13);rewind(f);assert(pause_read(f,17,10,ranges,&removed)==0);fclose(f);
 f=tmpfile();for(int i=0;i<65;i++)fprintf(f,"freeze_start: %d\nfreeze_end: %d\n",i*10,i*10+5);rewind(f);assert(pause_read(f,650,1,ranges,&removed)==-1);fclose(f);puts("PASS: pause interval parsing, trailing freeze, unchanged short runs and overflow");
}
