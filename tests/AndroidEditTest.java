package ru.screenrecorder.app;
import java.util.*;
public class AndroidEditTest {
 static void check(boolean value){if(!value)throw new AssertionError();}
 public static void main(String[] args){
  check(EditPlan.clock(83.45).equals("01:23.450"));check(EditPlan.clock(59.9996).equals("01:00.000"));check(EditPlan.clock(3601.234).equals("01:00:01.234"));
  List<double[]> source=Arrays.asList(new double[]{6,8},new double[]{1,3},new double[]{2,4},new double[]{4,5});
  List<double[]> keep=EditPlan.keep(source,12,false);check(keep.size()==2&&keep.get(0)[0]==1&&keep.get(0)[1]==5);
  List<double[]> outside=EditPlan.keep(source,12,true);check(outside.size()==3&&outside.get(0)[0]==0&&outside.get(0)[1]==1&&outside.get(2)[0]==8&&outside.get(2)[1]==12);
  check(EditPlan.keep(Collections.singletonList(new double[]{0,12}),12,true).isEmpty());
  boolean invalid=false;try{EditPlan.keep(Collections.singletonList(new double[]{3,2}),12,false);}catch(IllegalArgumentException e){invalid=true;}check(invalid);
  check(AudioMix.sample((short)16000,(short)12000,.5f,.25f,true,false)==8000);
  check(AudioMix.sample((short)16000,(short)12000,.5f,.25f,false,true)==3000);
  check(AudioMix.sample((short)16000,(short)12000,.5f,.25f,false,false)==0);
  check(AudioMix.sample((short)30000,(short)30000,1,1,true,true)==32767);
  List<double[]> cuts=PausePlan.cuts(Arrays.asList(new double[]{2,7},new double[]{9,19},new double[]{20,20.5}),22,1);
  check(cuts.size()==2&&cuts.get(0)[0]==3&&cuts.get(0)[1]==7&&cuts.get(1)[0]==10&&cuts.get(1)[1]==19);
  check(PausePlan.cuts(Collections.singletonList(new double[]{0,5}),5,5).isEmpty());
  check(PausePlan.same(new int[]{0xff223344},new int[]{0xff223344}));
  check(!PausePlan.same(new int[]{0xff223344},new int[]{0xffeeeeee}));
  List<double[]> safe=PausePlan.protect(Collections.singletonList(new double[]{1,10}),Arrays.asList(new double[]{2,4},new double[]{3,5},new double[]{7,8}),10);
  check(safe.size()==3&&safe.get(0)[0]==1&&safe.get(0)[1]==2&&safe.get(1)[0]==5&&safe.get(1)[1]==7&&safe.get(2)[0]==8&&safe.get(2)[1]==10);
  check(PausePlan.protect(Collections.singletonList(new double[]{1,10}),Collections.singletonList(new double[]{0,10}),10).isEmpty());
  check(PausePlan.protect(Collections.singletonList(new double[]{1,10}),Collections.emptyList(),10).size()==1);
  System.out.println("PASS: Android edit intervals, timestamps, automatic pause tails, microphone/system mute, gain and clipping");
 }
}
