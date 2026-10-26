import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewScript extends GhidraScript {
 public void run() throws Exception {
  String[] a={"00410590","00410880","004100b0","00443ff0","004436f0"};int[] n={750,318,25,0x17d0,270};
  Path dir=Paths.get("D:/Decomp/Bhunter/META/m4/resume400/a/original");Files.createDirectories(dir);
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
