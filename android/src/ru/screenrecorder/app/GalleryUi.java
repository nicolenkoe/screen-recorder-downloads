package ru.screenrecorder.app;
import android.app.*;import android.content.*;import android.database.Cursor;import android.graphics.*;import android.net.Uri;import android.os.*;import android.provider.*;import android.util.Size;import android.view.*;import android.widget.*;import org.json.*;import java.util.*;import java.util.concurrent.*;
final class GalleryUi {
 static final int IMPORT=31,DELETE=32;
 final MainActivity a;final ArrayList<Item> files=new ArrayList<>();final LinkedHashSet<Uri> selected=new LinkedHashSet<>();boolean choosing;int view=2;volatile int epoch;Button choose,delete,more;LinearLayout content;
 final ExecutorService thumbnails=Executors.newFixedThreadPool(2);
 static final class Item{Uri uri;String name;long duration,size,date;boolean imported;Item(Uri u,String n,long d,long s,long t,boolean x){uri=u;name=n;duration=d;size=s;date=t;imported=x;}}
 GalleryUi(MainActivity activity){a=activity;}
 void close(){epoch++;thumbnails.shutdownNow();}
 void hide(){epoch++;}
 ArrayList<Uri> imported(){
  ArrayList<Uri> out=new ArrayList<>();try{JSONArray j=new JSONArray(a.prefs.getString("imports","[]"));for(int i=0;i<j.length();i++)out.add(Uri.parse(j.getString(i)));}catch(Exception ignored){}return out;
 }
 void saveImports(Collection<Uri> values){JSONArray j=new JSONArray();for(Uri u:values)j.put(u.toString());a.prefs.edit().putString("imports",j.toString()).apply();}
 void addImports(Intent data){
  ArrayList<Uri> list=imported();ArrayList<Uri> found=new ArrayList<>();if(data.getData()!=null)found.add(data.getData());if(data.getClipData()!=null)for(int i=0;i<data.getClipData().getItemCount();i++)found.add(data.getClipData().getItemAt(i).getUri());
  for(Uri u:found){try{a.getContentResolver().takePersistableUriPermission(u,data.getFlags()&(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION));}catch(Exception ignored){}if(!list.contains(u))list.add(u);}saveImports(list);show();if(!found.isEmpty()){Item item=item(found.get(0));hide();a.viewer(found.get(0),item==null?"Добавленное видео":item.name);}
 }
 void importVideo(){Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("video/*").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true);a.startActivityForResult(i,IMPORT);}
 Button small(String label,Runnable action){Button b=new Button(a);b.setText(label);b.setAllCaps(false);b.setTextColor(MainActivity.TEXT);b.setTextSize(14*a.font());b.setBackground(a.round(MainActivity.SURFACE));b.setOnClickListener(v->action.run());return b;}
 void show(){
  epoch++;selected.clear();choosing=false;a.base("Галерея");TextView back=a.text("← К записи",14);back.setOnClickListener(v->{hide();a.home();});a.body.addView(back);
  a.button("+ Добавить видео",()->importVideo(),false);
  LinearLayout tools=new LinearLayout(a);choose=small("Выбрать",()->{choosing=!choosing;if(!choosing)selected.clear();render();});delete=small("Удалить",()->delete(new ArrayList<>(selected)));more=small("Ещё",()->actions(more,new ArrayList<>(selected)));
  for(Button b:new Button[]{choose,delete,more}){LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,a.dp(48),1);lp.setMargins(a.dp(3),a.dp(5),a.dp(3),a.dp(5));tools.addView(b,lp);}a.body.addView(tools);
  Spinner modes=new Spinner(a);modes.setAdapter(new ArrayAdapter<String>(a,android.R.layout.simple_spinner_dropdown_item,new String[]{"Список","Таблица","Миниатюры"}));view=a.prefs.getInt("galleryView",2);modes.setSelection(view);a.body.addView(modes);
  content=new LinearLayout(a);content.setOrientation(1);a.body.addView(content);
  readFiles();render();modes.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){public void onNothingSelected(AdapterView<?> p){}public void onItemSelected(AdapterView<?> p,View v,int n,long id){if(view!=n){view=n;a.prefs.edit().putInt("galleryView",n).apply();render();}}});
 }
 void readFiles(){
  files.clear();
  try(Cursor c=a.getContentResolver().query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,new String[]{"_id","_display_name","duration","_size","date_added"},"relative_path LIKE ? AND is_pending=0",new String[]{"Movies/ScreenRecorder%"},"date_added DESC")){
   while(c!=null&&c.moveToNext())files.add(new Item(ContentUris.withAppendedId(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,c.getLong(0)),c.getString(1),c.getLong(2),c.getLong(3),c.getLong(4)*1000,false));
  }catch(Exception e){a.message("Галерея: "+e.getMessage());}
  for(Uri uri:imported()){
   boolean exists=false;for(Item i:files)if(i.uri.equals(uri))exists=true;if(exists)continue;
   String name="Добавленное видео";long size=0;
   try(Cursor c=a.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE},null,null,null)){if(c!=null&&c.moveToFirst()){name=c.getString(0);size=c.isNull(1)?0:c.getLong(1);}}
   catch(Exception ignored){}
   files.add(new Item(uri,name,0,size,0,true));
  }
 }
 void render(){
  int generation=++epoch;content.removeAllViews();choose.setText(choosing?"Готово ("+selected.size()+")":"Выбрать");delete.setEnabled(!selected.isEmpty());more.setEnabled(!selected.isEmpty());
  if(files.isEmpty()){content.addView(a.text("Пока нет видео. Добавьте файл или запишите экран.",18));return;}
  if(view==1)content.addView(a.text("Название · длительность · размер",14));
  LinearLayout row=null;
  for(int index=0;index<files.size();index++){
   Item item=files.get(index);LinearLayout card=new LinearLayout(a);card.setOrientation(1);card.setPadding(a.dp(10),a.dp(10),a.dp(10),a.dp(10));card.setBackground(a.round(MainActivity.SURFACE));
   if(view==2){
    if(index%2==0){row=new LinearLayout(a);content.addView(row,new LinearLayout.LayoutParams(-1,-2));}
    ImageView image=new ImageView(a);image.setBackgroundColor(MainActivity.BG);image.setScaleType(ImageView.ScaleType.CENTER_CROP);image.setContentDescription("Кадр видео "+item.name);card.addView(image,new LinearLayout.LayoutParams(-1,a.dp(100)));
    thumbnails.execute(()->{if(epoch!=generation)return;try{Bitmap bitmap=a.getContentResolver().loadThumbnail(item.uri,new Size(160,90),null);a.runOnUiThread(()->{if(epoch==generation)image.setImageBitmap(bitmap);else bitmap.recycle();});}catch(Exception ignored){}catch(OutOfMemoryError ignored){}});
   }
   TextView title=a.text(item.name,view==2?15:17);title.setMaxLines(view==2?2:3);card.addView(title);
   String details=(item.duration>0?EditPlan.clock(item.duration/1000.)+" · ":"")+String.format(Locale.getDefault(),"%.1f МБ",item.size/1048576.)+(item.imported?" · внешний файл":"");
   if(view!=0)card.addView(a.text(details,12));
   if(choosing){CheckBox check=new CheckBox(a);check.setText("Выбрать");check.setTextColor(MainActivity.TEXT);check.setChecked(selected.contains(item.uri));check.setOnCheckedChangeListener((v,b)->{if(b)selected.add(item.uri);else selected.remove(item.uri);choose.setText("Готово ("+selected.size()+")");delete.setEnabled(!selected.isEmpty());more.setEnabled(!selected.isEmpty());});card.addView(check);}
   card.setOnClickListener(v->{if(choosing){if(selected.contains(item.uri))selected.remove(item.uri);else selected.add(item.uri);render();}else{hide();a.viewer(item.uri,item.name);}});
   card.setOnLongClickListener(v->{actions(card,Collections.singletonList(item.uri));return true;});
   LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(view==2?0:-1,-2,view==2?1:0);lp.setMargins(a.dp(4),a.dp(6),a.dp(4),a.dp(6));if(view==2)row.addView(card,lp);else content.addView(card,lp);
  }
 }
 Item item(Uri u){for(Item i:files)if(i.uri.equals(u))return i;return null;}
 void actions(View anchor,List<Uri> targets){
  if(targets.isEmpty())return;PopupMenu p=new PopupMenu(a,anchor);if(targets.size()==1){p.getMenu().add("Посмотреть").setOnMenuItemClickListener(m->{Item i=item(targets.get(0));hide();a.viewer(targets.get(0),i==null?"Видео":i.name);return true;});p.getMenu().add("Переименовать").setOnMenuItemClickListener(m->{rename(targets.get(0));return true;});}
  p.getMenu().add("Выбрать").setOnMenuItemClickListener(m->{choosing=true;selected.addAll(targets);render();return true;});
  p.getMenu().add("Поделиться").setOnMenuItemClickListener(m->{Intent i=new Intent(targets.size()>1?Intent.ACTION_SEND_MULTIPLE:Intent.ACTION_SEND).setType("video/mp4").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);if(targets.size()>1)i.putParcelableArrayListExtra(Intent.EXTRA_STREAM,new ArrayList<>(targets));else i.putExtra(Intent.EXTRA_STREAM,targets.get(0));a.startActivity(Intent.createChooser(i,"Поделиться видео"));return true;});
  p.getMenu().add("Убрать внешние видео из галереи").setOnMenuItemClickListener(m->{ArrayList<Uri> list=imported();list.removeAll(targets);saveImports(list);show();return true;});
  p.getMenu().add("Удалить файл"+(targets.size()>1?"ы":"")+"…").setOnMenuItemClickListener(m->{delete(targets);return true;});p.show();
 }
 void rename(Uri uri){
  Item i=item(uri);EditText input=new EditText(a);input.setSingleLine(true);input.setText(i==null?"Видео.mp4":i.name);new AlertDialog.Builder(a).setTitle("Переименовать видео").setView(input).setNegativeButton("Отмена",null).setPositiveButton("Сохранить",(d,w)->{
   String name=input.getText().toString().trim();if(name.isEmpty()||name.contains("/")||name.contains("\\")){a.message("Проверьте имя файла");return;}
   try{if(DocumentsContract.isDocumentUri(a,uri)){Uri next=DocumentsContract.renameDocument(a.getContentResolver(),uri,name);if(next!=null){ArrayList<Uri> imports=imported();int index=imports.indexOf(uri);if(index>=0){imports.set(index,next);saveImports(imports);}}}else{ContentValues values=new ContentValues();values.put(MediaStore.Video.Media.DISPLAY_NAME,name);if(a.getContentResolver().update(uri,values,null,null)<1)throw new Exception("Нет разрешения");}show();}catch(Exception e){a.message("Не удалось переименовать: "+e.getMessage());}
  }).show();
 }
 void delete(List<Uri> list){
  if(list.isEmpty())return;ArrayList<Uri> targets=new ArrayList<>(list);
  new AlertDialog.Builder(a).setTitle("Удалить видео?").setMessage("Файлов: "+targets.size()+". Для внешнего видео это удаление оригинала. Чтобы оставить файл, выберите «Убрать из галереи».").setNegativeButton("Отмена",null).setPositiveButton("Удалить",(d,w)->{
   ArrayList<Uri> system=new ArrayList<>();ArrayList<Uri> imports=imported();
   for(Uri uri:targets)try{
    if(DocumentsContract.isDocumentUri(a,uri)){if(!DocumentsContract.deleteDocument(a.getContentResolver(),uri))throw new Exception("Провайдер не разрешил удаление");imports.remove(uri);}
    else{try{a.getContentResolver().delete(uri,null,null);}catch(SecurityException e){if(Build.VERSION.SDK_INT>=30)system.add(uri);else if(e instanceof RecoverableSecurityException)a.startIntentSenderForResult(((RecoverableSecurityException)e).getUserAction().getActionIntent().getIntentSender(),DELETE,null,0,0,0);else throw e;}}
   }catch(Exception e){a.message("Не удалось удалить файл: "+e.getMessage());}
   saveImports(imports);
   if(!system.isEmpty()&&Build.VERSION.SDK_INT>=30)try{a.startIntentSenderForResult(MediaStore.createDeleteRequest(a.getContentResolver(),system).getIntentSender(),DELETE,null,0,0,0);}catch(Exception e){a.message("Удаление: "+e.getMessage());}
   show();
  }).show();
 }
}
