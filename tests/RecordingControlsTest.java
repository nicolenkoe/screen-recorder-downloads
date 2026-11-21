package ru.screenrecorder.app;
public final class RecordingControlsTest {
 static final class Fake implements RecordingControls.Target {
  boolean ready,paused,finishing;int stops;
  public boolean ready(){return ready&&!finishing;}
  public boolean paused(){return paused;}
  public boolean canStop(){return !finishing;}
  public void pause(boolean v){paused=v;}
  public void stop(){finishing=true;stops++;}
 }
 static void check(boolean b){if(!b)throw new AssertionError();}
 public static void main(String[] args){
  check(RecordingControls.state(null)==RecordingControls.State.IDLE);check(!RecordingControls.dispatch(null,"pause"));
  Fake t=new Fake();check(RecordingControls.state(t)==RecordingControls.State.PREPARING);check(!RecordingControls.dispatch(t,"pause"));check(!RecordingControls.dispatch(t,"resume"));
  t.ready=true;check(RecordingControls.state(t)==RecordingControls.State.RECORDING);
  check(RecordingControls.dispatch(t,"pause"));check(RecordingControls.state(t)==RecordingControls.State.PAUSED);
  check(RecordingControls.dispatch(t,"pause")&&t.paused); // double tap cannot resume
  check(RecordingControls.dispatch(t,"resume")&&!t.paused);check(RecordingControls.dispatch(t,"resume")&&!t.paused);
  RecordingControls.dispatch(t,"pause");check(RecordingControls.dispatch(t,"stop")&&t.stops==1);check(RecordingControls.state(t)==RecordingControls.State.SAVING);check(!RecordingControls.dispatch(t,"resume"));check(!RecordingControls.dispatch(t,"stop")&&t.stops==1);
  Fake preparing=new Fake();check(RecordingControls.dispatch(preparing,"stop")); // can cancel startup too
  check(!RecordingControls.dispatch(new Fake(),"unknown"));
  System.out.println("PASS: explicit pause/resume, repeated taps, stop from pause/preparation, state transitions and saving lock");
 }
}
