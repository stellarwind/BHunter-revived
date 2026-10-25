import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.symbol.SourceType;
public class WagonReviewedBoundaries extends GhidraScript {
 public void run() throws Exception {
  long[] begin={0x446160L,0x4462f0L,0x446b10L,0x446e00L};int[] bytes={395,2073,746,660};
  for(int i=0;i<begin.length;i++){
   Address a=toAddr(begin[i]);Address z=a.add(bytes[i]-1);
   if(currentProgram.getFunctionManager().getFunctionAt(a)!=null){println("already exists "+a);continue;}
   if(currentProgram.getFunctionManager().getFunctionContaining(a)!=null)throw new Exception("overlap "+a);
   println(currentProgram.getFunctionManager().createFunction("rail_wagon_"+a,a,new AddressSet(a,z),SourceType.USER_DEFINED).toString());
  }
 }
}
