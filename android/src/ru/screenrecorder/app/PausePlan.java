package ru.screenrecorder.app;
import java.util.*;
/** Converts detected static runs to removable tails; never removes the retained prefix. */
final class PausePlan {
 static ArrayList<double[]> cuts(List<double[]> runs,double duration,double retain){
  if(!Double.isFinite(retain)||retain<.1||retain>600)throw new IllegalArgumentException("Укажите время от 0,1 до 600 секунд");
  ArrayList<double[]> result=new ArrayList<>();
  for(double[] run:runs){double from=Math.max(0,run[0])+retain,to=Math.min(duration,run[1]);if(to>from+.001){if(result.size()==EditPlan.MAX)throw new IllegalArgumentException("Найдено больше 64 пауз. Обработайте видео по частям.");result.add(new double[]{from,to});}}
  return result;
 }
 static ArrayList<double[]> protect(List<double[]> cuts,List<double[]> sound,double duration){
  if(sound.isEmpty())return new ArrayList<>(cuts);
  ArrayList<double[]> guarded=new ArrayList<>(sound),out=new ArrayList<>();Collections.sort(guarded,(a,b)->Double.compare(a[0],b[0]));
  for(double[] cut:cuts){double at=cut[0];for(double[] r:guarded){if(r[1]<=at)continue;if(r[0]>=cut[1])break;if(r[0]>at)add(out,at,Math.min(r[0],cut[1]));at=Math.max(at,r[1]);if(at>=cut[1])break;}if(at<cut[1])add(out,at,cut[1]);}return out;
 }
 static void add(ArrayList<double[]> out,double a,double b){if(b<=a+.001)return;if(out.size()==EditPlan.MAX)throw new IllegalArgumentException("Больше 64 вырезок. Обработайте видео по частям.");out.add(new double[]{a,b});}
 static boolean same(int[] a,int[] b){
  if(a.length!=b.length)return false;long error=0;int changed=0;
  for(int i=0;i<a.length;i++){int d=Math.abs((a[i]>>16&255)-(b[i]>>16&255))+Math.abs((a[i]>>8&255)-(b[i]>>8&255))+Math.abs((a[i]&255)-(b[i]&255));error+=d;if(d>24)changed++;}
  return error<=a.length*3L&&changed<=a.length/500;
 }
}
