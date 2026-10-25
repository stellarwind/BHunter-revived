import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
public class ReviewInputMessages extends GhidraScript {
    public void run() throws Exception {
        Address a=toAddr("00473c40");
        disassemble(a);
        Function f=getFunctionAt(a);
        if(f==null)f=createFunction(a,"BH_InputWindowProc");
        println(f==null?"No function created":f.toString());
        println("Analysis metadata only; original executable is immutable.");
    }
}
