import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public class ReviewNetworkStartup extends GhidraScript {
 public void run() throws Exception {
  String[] addresses={"00475d60","00475ef0"};int[] lengths={398,317};
  Path directory=Paths.get("D:/Decomp/Bhunter/META/m4/hour407/next-review");Files.createDirectories(directory);
  for(int i=0;i<addresses.length;i++){
   Address address=toAddr(addresses[i]);Function function=getFunctionAt(address);
   if(function==null&&getFunctionContaining(address)==null)function=currentProgram.getFunctionManager().createFunction("FUN_"+addresses[i],address,new AddressSet(address,address.add(lengths[i]-1)),ghidra.program.model.symbol.SourceType.USER_DEFINED);
   disassemble(address);StringBuilder out=new StringBuilder("[");boolean first=true;
   InstructionIterator instructions=currentProgram.getListing().getInstructions(function.getBody(),true);
   while(instructions.hasNext()){
    Instruction ins=instructions.next();StringBuilder bytes=new StringBuilder();for(byte b:ins.getBytes())bytes.append(String.format("%02x",b&255));
    if(!first)out.append(",");first=false;out.append("{\"address\":\"").append(ins.getAddress()).append("\",\"bytes\":\"").append(bytes).append("\",\"assembly\":\"").append(ins.toString().replace("\\","\\\\").replace("\"","\\\"")).append("\"}");
   }
   Files.write(directory.resolve(addresses[i]+".disasm.json"),out.append("]\n").toString().getBytes(StandardCharsets.UTF_8));println(addresses[i]+" instructions exported");
  }
 }
}
