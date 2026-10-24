import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
public class ExportQueuedResource extends GhidraScript {
 public void run() throws Exception {
  Path out=Paths.get(getScriptArgs()[0]);Files.createDirectories(out);
  DecompInterface decomp=new DecompInterface();decomp.openProgram(currentProgram);
  for(String address:Arrays.copyOfRange(getScriptArgs(),1,getScriptArgs().length)) {
   Function function=getFunctionAt(toAddr(address));if(function==null)throw new Exception("Missing "+address);
   DecompileResults result=decomp.decompileFunction(function,30,monitor);
   if(!result.decompileCompleted())throw new Exception("Decompile failed "+address);
   Files.writeString(out.resolve(address+".c.txt"),result.getDecompiledFunction().getC(),StandardCharsets.UTF_8);
   List<String> lines=new ArrayList<>();lines.add("address\tbytes\tinstruction");
   InstructionIterator instructions=currentProgram.getListing().getInstructions(function.getBody(),true);
   while(instructions.hasNext()){
    Instruction ins=instructions.next();StringBuilder bytes=new StringBuilder();for(byte value:ins.getBytes())bytes.append(String.format("%02x",value&255));
    lines.add(ins.getAddress()+"\t"+bytes+"\t"+ins.toString());
   }
   Files.write(out.resolve(address+".tsv"),lines,StandardCharsets.UTF_8);
  }
  decomp.dispose();
 }
}
