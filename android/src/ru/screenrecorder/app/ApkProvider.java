package ru.screenrecorder.app;
import android.content.*;import android.database.*;import android.net.Uri;import android.os.*;import android.provider.OpenableColumns;import java.io.*;
public final class ApkProvider extends ContentProvider{
 public boolean onCreate(){return true;}File file(Uri uri)throws FileNotFoundException{try{File root=new File(getContext().getFilesDir(),"updates").getCanonicalFile();File file=new File(root,uri.getLastPathSegment()).getCanonicalFile();if(!root.equals(file.getParentFile())||!file.getName().endsWith(".apk"))throw new IOException();return file;}catch(Exception e){throw new FileNotFoundException();}}
 public ParcelFileDescriptor openFile(Uri u,String mode)throws FileNotFoundException{if(!"r".equals(mode))throw new FileNotFoundException();return ParcelFileDescriptor.open(file(u),ParcelFileDescriptor.MODE_READ_ONLY);}
 public String getType(Uri u){return "application/vnd.android.package-archive";}
 public Cursor query(Uri u,String[] p,String s,String[] a,String sort){try{File f=file(u);MatrixCursor c=new MatrixCursor(new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE});c.addRow(new Object[]{f.getName(),f.length()});return c;}catch(Exception e){return null;}}
 public Uri insert(Uri u,ContentValues v){throw new UnsupportedOperationException();}public int update(Uri u,ContentValues v,String s,String[]a){throw new UnsupportedOperationException();}public int delete(Uri u,String s,String[]a){throw new UnsupportedOperationException();}
}
