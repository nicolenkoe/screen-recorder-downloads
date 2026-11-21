package ru.screenrecorder.app;
import android.content.Context;import android.net.Uri;import android.media.*;import java.nio.*;import java.util.*;
/** Conservative protection: every decoded non-silent audio buffer, including speech. */
final class SoundProtection {
 static ArrayList<double[]> find(Context context,Uri input,double duration)throws Exception{
  MediaExtractor ex=new MediaExtractor();MediaCodec codec=null;ArrayList<double[]> sound=new ArrayList<>();
  try{ex.setDataSource(context,input,null);int track=-1;MediaFormat format=null;for(int i=0;i<ex.getTrackCount();i++){MediaFormat f=ex.getTrackFormat(i);if(f.getString(MediaFormat.KEY_MIME).startsWith("audio/")){track=i;format=f;break;}}
   if(track<0)return sound;ex.selectTrack(track);codec=MediaCodec.createDecoderByType(format.getString(MediaFormat.KEY_MIME));codec.configure(format,null,null,0);codec.start();
   int channels=format.getInteger(MediaFormat.KEY_CHANNEL_COUNT),rate=format.getInteger(MediaFormat.KEY_SAMPLE_RATE),encoding=AudioFormat.ENCODING_PCM_16BIT;boolean eos=false;MediaCodec.BufferInfo info=new MediaCodec.BufferInfo();long deadline=android.os.SystemClock.elapsedRealtime()+15000;
   while(true){if(Thread.currentThread().isInterrupted())throw new InterruptedException();if(android.os.SystemClock.elapsedRealtime()>deadline)throw new Exception("Тайм-аут анализа звука. Выделение не изменено.");
    if(!eos){int n=codec.dequeueInputBuffer(10000);if(n>=0){ByteBuffer b=codec.getInputBuffer(n);int size=ex.readSampleData(b,0);if(size<0){codec.queueInputBuffer(n,0,0,0,MediaCodec.BUFFER_FLAG_END_OF_STREAM);eos=true;}else{codec.queueInputBuffer(n,0,size,ex.getSampleTime(),0);ex.advance();deadline=android.os.SystemClock.elapsedRealtime()+15000;}}}
    int n=codec.dequeueOutputBuffer(info,10000);if(n==MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){MediaFormat f=codec.getOutputFormat();channels=f.getInteger(MediaFormat.KEY_CHANNEL_COUNT);rate=f.getInteger(MediaFormat.KEY_SAMPLE_RATE);if(f.containsKey(MediaFormat.KEY_PCM_ENCODING))encoding=f.getInteger(MediaFormat.KEY_PCM_ENCODING);continue;}
    if(n>=0){if(encoding!=AudioFormat.ENCODING_PCM_16BIT&&encoding!=AudioFormat.ENCODING_PCM_FLOAT)throw new Exception("Неподдерживаемый формат звука. Защита речи не выполнена.");
     int bytes=encoding==AudioFormat.ENCODING_PCM_FLOAT?4:2;ByteBuffer b=codec.getOutputBuffer(n).duplicate().order(ByteOrder.LITTLE_ENDIAN);b.position(info.offset);b.limit(info.offset+info.size);boolean heard=false;
     while(b.remaining()>=bytes){float v=bytes==4?b.getFloat():b.getShort()/32768f;if(!Float.isFinite(v))throw new Exception("Повреждённый звук");if(Math.abs(v)>.00316228f)heard=true;}
     if(heard){double a=Math.max(0,info.presentationTimeUs/1e6-.25),z=Math.min(duration,info.presentationTimeUs/1e6+info.size/(double)(bytes*channels*rate)+.25);if(z>a){if(!sound.isEmpty()&&a<=sound.get(sound.size()-1)[1])sound.get(sound.size()-1)[1]=Math.max(z,sound.get(sound.size()-1)[1]);else{if(sound.size()>=100000)throw new Exception("Слишком много звуковых участков");sound.add(new double[]{a,z});}}}
     boolean done=(info.flags&MediaCodec.BUFFER_FLAG_END_OF_STREAM)!=0;codec.releaseOutputBuffer(n,false);deadline=android.os.SystemClock.elapsedRealtime()+15000;if(done)break;
    }
   }return sound;
  }finally{ex.release();if(codec!=null){try{codec.stop();}catch(Exception ignored){}codec.release();}}
 }
}
