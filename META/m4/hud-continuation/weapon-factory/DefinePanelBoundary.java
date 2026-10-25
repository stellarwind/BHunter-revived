import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
public class DefinePanelBoundary extends GhidraScript {
    public void run() throws Exception {
        long[][] bounds={{0x00442440L,0x004428a9L},{0x00442b60L,0x00442be9L}};
        String[] names={"panel_refresh_00442440","panel_entry_00442b60"};
        for(int i=0;i<bounds.length;i++){
            Address start=toAddr(bounds[i][0]),end=toAddr(bounds[i][1]);
            Function f=currentProgram.getFunctionManager().getFunctionAt(start);
            if(f==null){
                int transaction=currentProgram.startTransaction("Reviewed panel boundary");
                try {f=currentProgram.getFunctionManager().createFunction(names[i],start,
                    new AddressSet(start,end),SourceType.USER_DEFINED);}
                finally {currentProgram.endTransaction(transaction,true);}
            }
            println(f+" "+f.getBody());
        }
    }
}
