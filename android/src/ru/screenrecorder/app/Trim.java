package ru.screenrecorder.app;
import android.content.*;import android.media.*;import android.net.Uri;import android.os.ParcelFileDescriptor;import android.provider.MediaStore;import java.nio.*;import java.util.*;
final class Trim{
 static Uri copy(Context c,Uri input,double from,double to)throws Exception{return pieces(c,input,Arrays.asList(new double[]{from,to}),false);}
 static Uri pieces(Context c,Uri input,List<double[]> ranges,boolean single)throws Exception{
  MediaExtractor probe=new MediaExtractor();long length=0;int video=-1,capacity=1048576;HashMap<Integer,MediaFormat> formats=new HashMap<>();ArrayList<long[]> selected=new ArrayList<>();
  try{probe.setDataSource(c,input,null);for(int t=0;t<probe.getTrackCount();t++){MediaFormat f=probe.getTrackFormat(t);String mime=f.getString(MediaFormat.KEY_MIME);if(mime.startsWith("video/")||mime.startsWith("audio/")){formats.put(t,f);if(f.containsKey(MediaFormat.KEY_MAX_INPUT_SIZE))capacity=Math.min(33554432,Math.max(capacity,f.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE)));}if(mime.startsWith("video/")&&video<0){video=t;if(f.containsKey(MediaFormat.KEY_DURATION))length=f.getLong(MediaFormat.KEY_DURATION);}}
   if(video<0)throw new Exception("Нет видеодорожки");if(length<=0){MediaMetadataRetriever r=new MediaMetadataRetriever();try{r.setDataSource(c,input);length=Long.parseLong(r.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION))*1000;}finally{r.release();}}
   probe.selectTrack(video);
   for(double[] range:ranges){if(range[0]<0||range[1]<=range[0]||range[1]*1000000>length+200000)throw new Exception("Проверьте границы фрагмента");probe.seekTo((long)(range[0]*1000000),MediaExtractor.SEEK_TO_PREVIOUS_SYNC);long start=probe.getSampleTime(),end=Math.min(length,(long)(range[1]*1000000));if(start<0||start>=end)throw new Exception("Фрагмент слишком короткий: увеличьте его до следующего ключевого кадра");if(!selected.isEmpty()&&start<=selected.get(selected.size()-1)[1])selected.get(selected.size()-1)[1]=Math.max(end,selected.get(selected.size()-1)[1]);else selected.add(new long[]{start,end});}
  }finally{probe.release();}
  if(selected.isEmpty())throw new Exception("После обрезки не остаётся видео");
  ContentValues values=new ContentValues();values.put(MediaStore.Video.Media.DISPLAY_NAME,(single?"Фрагмент_":"Обрезка_")+System.currentTimeMillis()+".mp4");values.put(MediaStore.Video.Media.MIME_TYPE,"video/mp4");values.put(MediaStore.Video.Media.RELATIVE_PATH,"Movies/ScreenRecorder");values.put(MediaStore.Video.Media.IS_PENDING,1);Uri out=c.getContentResolver().insert(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,values);if(out==null)throw new Exception("Нет места для копии");
  boolean ok=false;MediaMuxer mux=null;
  try(ParcelFileDescriptor fd=c.getContentResolver().openFileDescriptor(out,"rw")){
   if(fd==null)throw new Exception("Нет доступа к копии");mux=new MediaMuxer(fd.getFileDescriptor(),MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);HashMap<Integer,Integer> tracks=new HashMap<>();for(Map.Entry<Integer,MediaFormat> e:formats.entrySet())tracks.put(e.getKey(),mux.addTrack(e.getValue()));
   MediaMetadataRetriever meta=new MediaMetadataRetriever();try{meta.setDataSource(c,input);String rotation=meta.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION);if(rotation!=null)mux.setOrientationHint(Integer.parseInt(rotation));}finally{meta.release();}
   mux.start();ByteBuffer buffer=ByteBuffer.allocateDirect(capacity);MediaCodec.BufferInfo info=new MediaCodec.BufferInfo();long offset=0;
   for(long[] range:selected){if(Thread.currentThread().isInterrupted())throw new InterruptedException("Сохранение отменено");
    MediaExtractor ex=new MediaExtractor();int frames=0;
    try{ex.setDataSource(c,input,null);for(int track:tracks.keySet())ex.selectTrack(track);ex.seekTo(range[0],MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
     while(ex.getSampleTrackIndex()>=0){if(Thread.currentThread().isInterrupted())throw new InterruptedException("Сохранение отменено");int track=ex.getSampleTrackIndex();long time=ex.getSampleTime();if(time>=range[1]){ex.unselectTrack(track);continue;}if(time>=range[0]){buffer.clear();int size=ex.readSampleData(buffer,0);if(size<0)break;if((ex.getSampleFlags()&MediaExtractor.SAMPLE_FLAG_ENCRYPTED)!=0)throw new Exception("Защищённое видео нельзя обрезать");info.set(0,size,offset+time-range[0],(ex.getSampleFlags()&MediaExtractor.SAMPLE_FLAG_SYNC)!=0?MediaCodec.BUFFER_FLAG_KEY_FRAME:0);mux.writeSampleData(tracks.get(track),buffer,info);if(track==video)frames++;}ex.advance();}
    }finally{ex.release();}
    if(frames==0)throw new Exception("Во фрагменте нет кадров");offset+=range[1]-range[0];
   }
   mux.stop();ok=true;
  }finally{if(mux!=null)mux.release();if(!ok)c.getContentResolver().delete(out,null,null);}
  values.clear();values.put(MediaStore.Video.Media.IS_PENDING,0);c.getContentResolver().update(out,values,null,null);return out;
 }
}
