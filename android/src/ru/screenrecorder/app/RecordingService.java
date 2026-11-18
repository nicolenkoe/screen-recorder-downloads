package ru.screenrecorder.app;
import android.app.*;import android.content.*;import android.content.pm.ServiceInfo;import android.media.projection.*;import android.os.*;
public final class RecordingService extends Service {
 public static volatile RecordingService active; public static volatile String status="Готово к записи",lastUri=""; public static volatile int level;
 private volatile RecorderEngine engine; private volatile boolean finishing;private final java.util.concurrent.CountDownLatch ready=new java.util.concurrent.CountDownLatch(1);private String error="";
 public void setGain(int percent){if(engine!=null)engine.gain=Math.max(0,Math.min(100,percent))/100f;}
 public android.os.IBinder onBind(Intent i){return null;}
 public void onCreate(){super.onCreate();((NotificationManager)getSystemService(NOTIFICATION_SERVICE)).createNotificationChannel(new NotificationChannel("recording","Запись экрана",NotificationManager.IMPORTANCE_LOW));}
 private PendingIntent action(String a){return PendingIntent.getService(this,a.hashCode(),new Intent(this,RecordingService.class).setAction(a),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);}
 private Notification notification(){return new Notification.Builder(this,"recording").setSmallIcon(android.R.drawable.presence_video_online).setContentTitle("Запись экрана").setContentText(status).setOngoing(true).setContentIntent(PendingIntent.getActivity(this,0,new Intent(this,MainActivity.class),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE)).addAction(new Notification.Action.Builder(null,engine!=null&&engine.paused?"Продолжить":"Пауза",action("pause")).build()).addAction(new Notification.Action.Builder(null,"Остановить",action("stop")).build()).build();}
 public int onStartCommand(Intent i,int flags,int id){
  if(i==null){stopSelf();return START_NOT_STICKY;}
  if("stop".equals(i.getAction())){finish();return START_NOT_STICKY;}
  if("pause".equals(i.getAction())){if(engine!=null){engine.setPaused(!engine.paused);status=engine.paused?"Запись на паузе":"Идёт запись";((NotificationManager)getSystemService(NOTIFICATION_SERVICE)).notify(7,notification());}return START_NOT_STICKY;}
  if(active!=null)return START_NOT_STICKY;
  active=this;status="Подготовка записи…";boolean mic=i.getBooleanExtra("mic",true),internal=i.getBooleanExtra("internal",false);
  int type=ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION;if(mic&&Build.VERSION.SDK_INT>=30)type|=ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE;
  try{startForeground(7,notification(),type);}catch(Exception e){status="Ошибка разрешений: "+e.getMessage();active=null;stopSelf();return START_NOT_STICKY;}
  final Intent request=i;
  new Thread(()->{try{
   MediaProjection projection=((MediaProjectionManager)getSystemService(MEDIA_PROJECTION_SERVICE)).getMediaProjection(request.getIntExtra("result",-1),(Intent)request.getParcelableExtra("data"));
   engine=new RecorderEngine(this,projection,mic,internal,request.getIntExtra("gain",100),request.getIntExtra("quality",0));
   engine.start();if(finishing)return;status="Идёт запись";((NotificationManager)getSystemService(NOTIFICATION_SERVICE)).notify(7,notification());
  }catch(Exception e){error="Запись не началась: "+e.getMessage();finish();}finally{ready.countDown();}},"capture-start").start();return START_NOT_STICKY;
 }
 public synchronized void finish(){if(finishing)return;finishing=true;status="Сохранение…";new Thread(()->{try{if(!ready.await(30,java.util.concurrent.TimeUnit.SECONDS))throw new Exception("Тайм-аут подготовки");if(engine!=null){android.net.Uri uri=engine.stop();if(uri!=null){lastUri=uri.toString();status="Видео сохранено";}}}catch(Exception e){status="Ошибка сохранения: "+e.getMessage();}finally{if("Сохранение…".equals(status))status="Запись остановлена";if(!error.isEmpty())status=error;active=null;level=0;stopForeground(STOP_FOREGROUND_REMOVE);stopSelf();}},"capture-stop").start();}
 public void onDestroy(){if(!finishing)finish();super.onDestroy();}
}
