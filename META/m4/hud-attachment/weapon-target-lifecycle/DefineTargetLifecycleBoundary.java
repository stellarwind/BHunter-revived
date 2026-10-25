import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefineTargetLifecycleBoundary extends GhidraScript {
    public void run() throws Exception {
        for(long[] range:new long[][]{{0x4310c0L,0x43112bL},{0x431180L,0x4314e6L}}) {
        Address start=toAddr(range[0]),end=toAddr(range[1]);
        Function f=currentProgram.getFunctionManager().getFunctionAt(start);
        if(f==null){
            int transaction=currentProgram.startTransaction("Reviewed complete target weapon lifecycle boundary");
            try {f=currentProgram.getFunctionManager().createFunction("weapon_target_"+start,start,new AddressSet(start,end),SourceType.USER_DEFINED);}
            finally {currentProgram.endTransaction(transaction,true);}
        }
        println(f+" "+f.getBody());
        }
    }
}

