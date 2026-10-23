import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class RecoverCallTargets extends GhidraScript {
    public void run() throws Exception {
        TreeSet<Address> targets=new TreeSet<>();
        InstructionIterator it=currentProgram.getListing().getInstructions(true);
        while(it.hasNext()) {
            Instruction ins=it.next();
            if(ins.getFlowType().isCall()) for(Address target:ins.getFlows()) {
                if(target.getOffset()>=0x401000L && target.getOffset()<0x488600L) targets.add(target);
            }
        }
        int created=0;
        for(Address target:targets) {
            monitor.checkCancelled();
            if(currentProgram.getFunctionManager().getFunctionContaining(target)==null && currentProgram.getListing().getInstructionAt(target)!=null) {
                Function f=createFunction(target,null);
                if(f!=null) {f.addTag("recovered-direct-call-target");created++;}
            }
        }
        println("Created "+created+" functions from decoded direct CALL destinations; no binary bytes changed.");
        TreeSet<Address> pointers=new TreeSet<>();
        ReferenceIterator refs=currentProgram.getReferenceManager().getReferenceIterator(currentProgram.getMinAddress());
        while(refs.hasNext()) {
            Reference ref=refs.next(); Address target=ref.getToAddress();
            if(ref.getReferenceType().isData() && target.getOffset()>=0x401000L && target.getOffset()<0x47d360L && (target.getOffset()&15)==0)
                pointers.add(target);
        }
        int callbacks=0;
        for(Address target:pointers) {
            monitor.checkCancelled();
            if(currentProgram.getFunctionManager().getFunctionContaining(target)==null && currentProgram.getListing().getInstructionAt(target)!=null) {
                try {
                    Function f=createFunction(target,null);
                    if(f!=null) {f.addTag("candidate-function-code-pointer");callbacks++;}
                } catch(Exception e) {println("Skipped "+target+": "+e.getMessage());}
            }
        }
        println("Created "+callbacks+" candidate callbacks from aligned decoded code pointers. Verify boundaries during decompilation.");
    }
}
