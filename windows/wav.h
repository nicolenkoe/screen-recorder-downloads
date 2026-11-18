#ifndef RECORDER_WAV_H
#define RECORDER_WAV_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#define wav_seek _fseeki64
#else
#define wav_seek fseeko
#endif
/* RF64 avoids the 4 GiB limit of ordinary WAV recordings. */
typedef struct { FILE *f; uint64_t bytes; uint32_t block; uint64_t header; int failed; } Wave;
static int wav_write(Wave *w,const void *p,size_t n) {
    if(w->failed || fwrite(p,1,n,w->f)!=n) { w->failed=1; return 0; } return 1;
}
static int wav_open(Wave *w,FILE *f,const void *fmt,uint32_t fmtSize,uint32_t block) {
    memset(w,0,sizeof(*w)); w->f=f; w->block=block;
    uint32_t max=UINT32_MAX,ds=28,zero=0; uint64_t empty=0;
    wav_write(w,"RF64",4); wav_write(w,&max,4); wav_write(w,"WAVEds64",8); wav_write(w,&ds,4);
    for(int i=0;i<3;i++) wav_write(w,&empty,8); wav_write(w,&zero,4);
    wav_write(w,"fmt ",4); wav_write(w,&fmtSize,4); wav_write(w,fmt,fmtSize);
    if(fmtSize&1) wav_write(w,&zero,1);
    wav_write(w,"data",4); wav_write(w,&max,4); w->header=12+8+28+8+fmtSize+(fmtSize&1)+8;
    return !w->failed;
}
static int wav_samples(Wave *w,const void *p,uint64_t frames) {
    static const unsigned char silence[8192]={0};
    uint64_t n=frames*w->block;
    if(p) { if(n>SIZE_MAX || !wav_write(w,p,(size_t)n)) return 0; }
    else { uint64_t left=n; while(left) { size_t part=left>sizeof(silence)?sizeof(silence):(size_t)left;
        if(!wav_write(w,silence,part)) return 0; left-=part; } }
    w->bytes+=n; return 1;
}
static int wav_checkpoint(Wave *w) {
    if(!w->f) return 0;
    uint64_t size=w->header+w->bytes-8,frames=w->bytes/w->block;
    if(wav_seek(w->f,20,SEEK_SET)!=0) w->failed=1;
    wav_write(w,&size,8); wav_write(w,&w->bytes,8); wav_write(w,&frames,8);
    if(wav_seek(w->f,(long long)(w->header+w->bytes),SEEK_SET)!=0 || fflush(w->f)!=0)w->failed=1;
    return !w->failed;
}
static int wav_close(Wave *w) {
    if(!w->f) return 0;
    wav_checkpoint(w);
    if(fclose(w->f)!=0) w->failed=1; w->f=NULL; return !w->failed;
}
#endif
