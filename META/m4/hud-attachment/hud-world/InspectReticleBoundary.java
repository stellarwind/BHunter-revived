import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
public class InspectReticleBoundary extends GhidraScript {
    public void run() throws Exception {
        Address start=toAddr(0x004314f0L);
        Function f=currentProgram.getFunctionManager().getFunctionContaining(start);
        println("containing="+f);
        if(f!=null)println("body="+f.getBody());
        println("entry_instruction="+getInstructionAt(start));
    }
}
