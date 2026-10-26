import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewActorRoute extends GhidraScript {
 public void run() throws Exception {
  String[] a={"004374b0","004375b0"};int[] n={242,291};
  Path dir=Paths.get("D:/Decomp/Bhunter/META/m4/target550/c3/original");Files.createDirectories(dir);
  for(int i=0;i<a.length;i++){
   Address p=toAddr(a[i]);Function f=getFunctionContaining(p);println(a[i]+" containing="+f);
   if(getFunctionAt(p)==null && f==null)currentProgram.getFunctionManager().createFunction("FUN_"+a[i],p,new AddressSet(p,p.add(n[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);
   f=getFunctionAt(p);if(f==null)continue;disassemble(p);
   StringBuilder out=new StringBuilder("[");boolean first=true;InstructionIterator iter=currentProgram.getListing().getInstructions(f.getBody(),true);
   while(iter.hasNext()){
    Instruction ins=iter.next();StringBuilder hex=new StringBuilder();for(byte b:ins.getBytes())hex.append(String.format("%02x",b&255));
    if(!first)out.append(",");first=false;out.append("{\"address\":\"").append(ins.getAddress()).append("\",\"bytes\":\"").append(hex).append("\",\"assembly\":\"").append(ins.toString().replace("\\","\\\\").replace("\"","\\\"")).append("\"}");
   }
   Files.write(dir.resolve(a[i]+".disasm.json"),out.append("]\n").toString().getBytes(StandardCharsets.UTF_8));
  }
 }
}
