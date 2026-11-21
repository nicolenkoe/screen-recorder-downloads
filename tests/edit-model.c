#include <assert.h>
#include <string.h>
#include "../windows/edit_model.h"
int main(int argc,char **argv){
 assert(edit_clamp(-1,12)==0&&edit_clamp(20,12)==12&&edit_clamp(NAN,12)==0);
 wchar_t text[80];edit_time(text,80,83.45,1);assert(!wcscmp(text,L"01:23.450"));edit_time(text,80,59.9996,1);assert(!wcscmp(text,L"01:00.000"));edit_time(text,80,3601.234,1);assert(!wcscmp(text,L"01:00:01.234"));edit_time(text,80,NAN,1);assert(!wcscmp(text,L"00:00.000"));assert(edit_tick_step(340,6)==60);
 EditRange ranges[]={{6,8},{1,3},{2,4},{4,5}},out[65];int n=edit_plan(ranges,4,12,0,out);assert(n==2&&out[0].start==1&&out[0].end==5&&out[1].start==6&&out[1].end==8);
 n=edit_plan(ranges,4,12,1,out);assert(n==3&&out[0].start==0&&out[0].end==1&&out[1].start==5&&out[1].end==6&&out[2].start==8&&out[2].end==12);
 EditRange full={0,12};assert(edit_plan(&full,1,12,1,out)==0);EditRange bad={4,3};assert(edit_plan(&bad,1,12,0,out)==-1);bad=(EditRange){NAN,3};assert(edit_plan(&bad,1,12,0,out)==-1);
 if(argc>1){FILE *f=fopen(argv[1],"wb");EditRange r[]={{1,3},{6,8}};n=edit_plan(r,2,12,argc>2&&!strcmp(argv[2],"remove"),out);assert(edit_write_graph(f,out,n,argc<4||strcmp(argv[3],"silent")));fclose(f);}
 puts("PASS: timestamps, rounding, hours, interval merge, keep/remove multiple pieces, invalid and empty edits");return 0;
}
