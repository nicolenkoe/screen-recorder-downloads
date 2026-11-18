package ru.screenrecorder.app;
import java.util.*;
public final class ReleasePolicy {
 public static final class Release {public final long published,version;public final int minSdk,maxSdk;public final boolean stable;public final String name;public Release(String n,long p,long v,int min,int max,boolean s){name=n;published=p;version=v;minSdk=min;maxSdk=max;stable=s;}}
 public static Release choose(List<Release> all,boolean stableOnly,int sdk){Release best=null;for(Release r:all)if((!stableOnly||r.stable)&&sdk>=r.minSdk&&(r.maxSdk==0||sdk<=r.maxSdk)&&(best==null||r.published>best.published||r.published==best.published&&r.version>best.version))best=r;return best;}
}
