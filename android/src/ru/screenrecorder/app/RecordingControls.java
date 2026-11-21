package ru.screenrecorder.app;
/** Explicit, idempotent commands shared by the activity and notification. */
final class RecordingControls {
 interface Target {boolean ready();boolean paused();boolean canStop();void pause(boolean value);void stop();}
 enum State {IDLE,PREPARING,RECORDING,PAUSED,SAVING}
 static State state(Target t){if(t==null)return State.IDLE;if(!t.canStop())return State.SAVING;if(!t.ready())return State.PREPARING;return t.paused()?State.PAUSED:State.RECORDING;}
 static boolean dispatch(Target t,String action){
  if(t==null)return false;
  if("stop".equals(action)){if(!t.canStop())return false;t.stop();return true;}
  if(!t.ready())return false;
  if("pause".equals(action)){t.pause(true);return true;}
  if("resume".equals(action)){t.pause(false);return true;}
  return false;
 }
}
