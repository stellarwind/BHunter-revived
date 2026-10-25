import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefineWeaponArcLifecycleBoundary extends GhidraScript {
    public void run() throws Exception {
        for(long[] range:new long[][]{{0x004324a0L,0x00432509L},{0x00432510L,0x00432550L},{0x00432c60L,0x00432fe3L}}){
        Address start=toAddr(range[0]),end=toAddr(range[1]);
        Function f=currentProgram.getFunctionManager().getFunctionAt(start);
        if(f==null){
            int transaction=currentProgram.startTransaction("Reviewed complete shared weapon state callbacks");
            try {
                f=currentProgram.getFunctionManager().createFunction("weapon_arc_lifecycle_"+start+"",start,new AddressSet(start,end),SourceType.USER_DEFINED);
            } finally {currentProgram.endTransaction(transaction,true);}
        }
        println(f+" "+f.getBody());
        }
    }
}
