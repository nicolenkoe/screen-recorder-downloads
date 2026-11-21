package ru.screenrecorder.app;
import java.util.*;
final class EditPlan {
 static final int MAX=64;
 static String clock(double seconds){long ms=Math.round(Math.max(0,seconds)*1000),s=ms/1000;return s>=3600?String.format(Locale.US,"%02d:%02d:%02d.%03d",s/3600,s/60%60,s%60,ms%1000):String.format(Locale.US,"%02d:%02d.%03d",s/60,s%60,ms%1000);}
 static ArrayList<double[]> keep(List<double[]> source,double duration,boolean remove){
  if(!Double.isFinite(duration)||duration<=0||source.isEmpty()||source.size()>MAX)throw new IllegalArgumentException("Неверное выделение");
  ArrayList<double[]> sorted=new ArrayList<>();for(double[] r:source){if(!Double.isFinite(r[0])||!Double.isFinite(r[1])||r[0]<0||r[1]<=r[0]||r[1]>duration+.000001)throw new IllegalArgumentException("Проверьте начало и конец");sorted.add(new double[]{r[0],Math.min(duration,r[1])});}
  Collections.sort(sorted,(a,b)->Double.compare(a[0],b[0]));ArrayList<double[]> merged=new ArrayList<>();
  for(double[] r:sorted){if(!merged.isEmpty()&&r[0]<=merged.get(merged.size()-1)[1]+.000001)merged.get(merged.size()-1)[1]=Math.max(r[1],merged.get(merged.size()-1)[1]);else merged.add(r);}
  if(!remove)return merged;ArrayList<double[]> out=new ArrayList<>();double at=0;for(double[] r:merged){if(r[0]>at+.000001)out.add(new double[]{at,r[0]});at=r[1];}if(at<duration-.000001)out.add(new double[]{at,duration});return out;
 }
}
