import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefinePlayerSceneBoundary extends GhidraScript {
    public void run() throws Exception {
        Address start=toAddr(0x0041c860L),end=toAddr(0x0041d7bdL);
        Function f=currentProgram.getFunctionManager().getFunctionAt(start);
        if(f==null){
            int transaction=currentProgram.startTransaction("Reviewed complete weapon target reticle boundary");
            try {
                f=currentProgram.getFunctionManager().createFunction("player_scene_0041c860",start,new AddressSet(start,end),SourceType.USER_DEFINED);
            } finally {currentProgram.endTransaction(transaction,true);}
        }
        println(f+" "+f.getBody());
    }
}


