import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class ReviewControl extends GhidraScript {
 public void run() throws Exception {
  String[] a={"00414880","00415530","00416eb0"};int[] n={3243,3042,3486};
  for(int i=0;i<a.length;i++){
   Address p=toAddr(a[i]);Function f=getFunctionContaining(p);println(a[i]+" containing="+f);
   if(getFunctionAt(p)==null && f==null)currentProgram.getFunctionManager().createFunction("FUN_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);
  }
 }
}
