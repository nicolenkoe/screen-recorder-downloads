package ru.screenrecorder.app;
import android.content.Context;import android.net.Uri;import android.media.MediaMetadataRetriever;import android.graphics.Bitmap;import java.util.*;
final class PauseDetector {
 static ArrayList<double[]> find(Context context,Uri input,double duration,double retain) throws Exception {
  MediaMetadataRetriever meta=new MediaMetadataRetriever();ArrayList<double[]> runs=new ArrayList<>();int[] anchor=null;double start=0,last=0;final double step=.2;
  try{meta.setDataSource(context,input);
   for(double at=0;at<duration;at+=step){
    if(Thread.currentThread().isInterrupted())throw new InterruptedException();
    Bitmap frame=meta.getScaledFrameAtTime((long)(at*1000000),MediaMetadataRetriever.OPTION_CLOSEST,320,180);
    if(frame==null)throw new Exception("Не удалось прочитать кадр. Выделение не изменено.");
    int[] pixels=new int[frame.getWidth()*frame.getHeight()];frame.getPixels(pixels,0,frame.getWidth(),0,0,frame.getWidth(),frame.getHeight());frame.recycle();
    if(anchor==null){anchor=pixels;start=at;}else if(!PausePlan.same(anchor,pixels)){
     // Conservative endpoint: do not remove the interval preceding a changed sample.
     if(last>start+retain)runs.add(new double[]{start,last});anchor=pixels;start=at;
    }last=at;
   }
   if(last>start+retain)runs.add(new double[]{start,last});
   return PausePlan.cuts(runs,duration,retain);
  }finally{meta.release();}
 }
}
