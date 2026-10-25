import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefineWeaponCommonBoundary extends GhidraScript {
    public void run() throws Exception {
        for(long[] range:new long[][]{{0x004309b0L,0x004309c5L},{0x00431050L,0x004310bbL}}){
        Address start=toAddr(range[0]),end=toAddr(range[1]);
        Function f=currentProgram.getFunctionManager().getFunctionAt(start);
        if(f==null){
            int transaction=currentProgram.startTransaction("Reviewed complete shared weapon state callbacks");
            try {
                f=currentProgram.getFunctionManager().createFunction("weapon_common_"+start+"",start,new AddressSet(start,end),SourceType.USER_DEFINED);
            } finally {currentProgram.endTransaction(transaction,true);}
        }
        println(f+" "+f.getBody());
        }
    }
}
