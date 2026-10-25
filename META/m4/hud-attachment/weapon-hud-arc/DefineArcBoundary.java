import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefineArcBoundary extends GhidraScript {
    public void run() throws Exception {
        Address start=toAddr(0x00432ff0L),end=toAddr(0x00433166L);
        Function f=currentProgram.getFunctionManager().getFunctionAt(start);
        if(f==null){
            int transaction=currentProgram.startTransaction("Reviewed complete weapon target reticle boundary");
            try {
                f=currentProgram.getFunctionManager().createFunction("weapon_hud_reticle_00432ff0",start,new AddressSet(start,end),SourceType.USER_DEFINED);
            } finally {currentProgram.endTransaction(transaction,true);}
        }
        println(f+" "+f.getBody());
    }
}

