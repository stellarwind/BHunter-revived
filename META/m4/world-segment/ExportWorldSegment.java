import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.io.File;
import java.io.FileWriter;

/* Read-only complete instruction export, including noncontiguous blocks. */
public class ExportWorldSegment extends GhidraScript {
    public void run() throws Exception {
        File out=new File(getScriptArgs()[0]);out.mkdirs();
        String[] addresses={"0041e6b0","0040c4a0","0040ba40","0040b020"};
        for(String address:addresses) {
            Function function=getFunctionAt(toAddr(address));
            if(function==null)throw new IllegalStateException(address);
            FileWriter writer=new FileWriter(new File(out,address+".tsv"));
            writer.write("address\tbytes\tinstruction\n");
            InstructionIterator instructions=currentProgram.getListing().getInstructions(function.getBody(),true);
            int bytes=0,count=0;
            while(instructions.hasNext()) {
                Instruction instruction=instructions.next();
                StringBuilder hex=new StringBuilder();
                for(byte value:instruction.getBytes())hex.append(String.format("%02x",value&255));
                writer.write(instruction.getAddress()+"\t"+hex+"\t"+instruction+"\n");
                bytes+=instruction.getLength();++count;
            }
            writer.close();
            println(address+" body="+function.getBody()+" instructions="+count+" bytes="+bytes);
        }
    }
}
