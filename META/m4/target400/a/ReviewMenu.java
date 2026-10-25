import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewMenu extends GhidraScript {
 public void run() throws Exception {
  String[] a={"0042ca30","0042caa0","0042cae0","0042cb10","0042e360","0042e540","0042e690","0042e7e0","0042ea70","0042eae0","0042ee50","0042f2b0"};
  int[] n={97,51,47,47,147,205,205,225,110,77,77,151};
  for(int i=0;i<a.length;i++){
   Address p=toAddr(a[i]);Function f=getFunctionContaining(p);println(a[i]+" containing="+f);
   if(getFunctionAt(p)==null && f==null) currentProgram.getFunctionManager().createFunction("FUN_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);
   f=getFunctionAt(p);
   if(f!=null){
    StringBuilder out=new StringBuilder("[");boolean first=true;
    InstructionIterator iter=currentProgram.getListing().getInstructions(f.getBody(),true);
    while(iter.hasNext()){
     Instruction ins=iter.next();StringBuilder hex=new StringBuilder();for(byte b:ins.getBytes())hex.append(String.format("%02x",b&255));
     if(!first)out.append(",");first=false;
     out.append("{\"address\":\"").append(ins.getAddress()).append("\",\"bytes\":\"").append(hex).append("\",\"assembly\":\"").append(ins.toString().replace("\\","\\\\").replace("\"","\\\"")).append("\"}");
    }
    out.append("]\n");
    Files.write(Paths.get("D:/Decomp/Bhunter/META/m4/target400/a/original/"+a[i]+".disasm.json"),out.toString().getBytes(StandardCharsets.UTF_8));
   }
  }
 }
}
