// Export observed function bodies and references; label byte-verified CRT matches.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class MapExecutable extends GhidraScript {
    String clean(String s) { return s.replace('\t',' ').replace('\n',' ').replace('\r',' '); }
    public void run() throws Exception {
        Path out = Paths.get(getScriptArgs()[0]);
        FunctionManager fm = currentProgram.getFunctionManager();
        Listing listing = currentProgram.getListing();
        int labelled = 0;
        for (String line : Files.readAllLines(out.resolve("crt-symbols.tsv"))) {
            String[] fields = line.split("\t");
            Address address = toAddr(fields[0]);
            String name = fields[1];
            Function f = fm.getFunctionAt(address);
            if (f == null && listing.getInstructionAt(address) != null && fm.getFunctionContaining(address) == null)
                f = createFunction(address, name);
            if (f != null) {
                if (f.getName().startsWith("FUN_")) f.setName(name, SourceType.USER_DEFINED);
                f.addTag("library-crt-verified");
                setPlateComment(address,"COFF code section matched VC5 CRT with relocation operands masked. See analysis/crt-comparison.json. Symbol: " + name);
                labelled++;
            } else createLabel(address,name,false);
        }
        List<String> rows = new ArrayList<>();
        rows.add("address\tname\tbody_bytes\tinstructions\tebp_frame\treturn_pops\tcalls\tstrings\ttags");
        List<String> edges = new ArrayList<>();
        edges.add("caller\tcallsite\ttarget\tname");
        FunctionIterator functions = fm.getFunctions(true);
        while (functions.hasNext()) {
            monitor.checkCancelled();
            Function f = functions.next();
            Set<String> calls = new TreeSet<>(), strings = new TreeSet<>(), pops = new TreeSet<>(), tags = new TreeSet<>();
            for (FunctionTag t : f.getTags()) tags.add(t.getName());
            int count = 0;
            boolean ebp = false;
            InstructionIterator instructions = listing.getInstructions(f.getBody(),true);
            while (instructions.hasNext()) {
                Instruction ins = instructions.next(); count++;
                String text = ins.toString().toUpperCase();
                if (count < 8 && text.startsWith("MOV EBP,ESP")) ebp = true;
                if (ins.getMnemonicString().equalsIgnoreCase("RET")) pops.add(clean(ins.toString()));
                for (Reference ref : ins.getReferencesFrom()) {
                    Address dest = ref.getToAddress();
                    if (ref.getReferenceType().isCall()) {
                        Function callee = fm.getFunctionAt(dest);
                        String name = callee == null ? dest.toString() : callee.getName();
                        calls.add(dest + ":" + name);
                        edges.add(f.getEntryPoint()+"\t"+ins.getAddress()+"\t"+dest+"\t"+clean(name));
                    }
                    if (ref.getReferenceType().isData()) {
                        Data d = listing.getDefinedDataAt(dest);
                        if (d != null && d.hasStringValue()) strings.add(dest+":"+clean(String.valueOf(d.getValue())));
                    }
                }
            }
            rows.add(f.getEntryPoint()+"\t"+f.getName()+"\t"+f.getBody().getNumAddresses()+"\t"+count+"\t"+ebp+"\t"+String.join("|",pops)+"\t"+String.join("|",calls)+"\t"+String.join("|",strings)+"\t"+String.join("|",tags));
        }
        Files.write(out.resolve("function-map.tsv"), rows, StandardCharsets.UTF_8);
        Files.write(out.resolve("call-edges.tsv"), edges, StandardCharsets.UTF_8);
        List<String> gaps = new ArrayList<>();
        gaps.add("start\tend\tinstructions\tbytes");
        Address gapStart=null, gapEnd=null;
        int gapCount=0, gapBytes=0, covered=0, uncovered=0;
        InstructionIterator allInstructions=listing.getInstructions(true);
        while(allInstructions.hasNext()) {
            Instruction ins=allInstructions.next();
            if(ins.getAddress().getOffset()<0x401000L || ins.getAddress().getOffset()>=0x488600L) continue;
            if(fm.getFunctionContaining(ins.getAddress())==null) {
                if(gapStart==null) gapStart=ins.getAddress();
                gapEnd=ins.getMaxAddress(); gapCount++; gapBytes+=ins.getLength(); uncovered+=ins.getLength();
            } else {
                covered+=ins.getLength();
                if(gapStart!=null) {
                    gaps.add(gapStart+"\t"+gapEnd+"\t"+gapCount+"\t"+gapBytes);
                    gapStart=null;gapCount=0;gapBytes=0;
                }
            }
        }
        if(gapStart!=null) gaps.add(gapStart+"\t"+gapEnd+"\t"+gapCount+"\t"+gapBytes);
        Files.write(out.resolve("unassigned-code.tsv"),gaps,StandardCharsets.UTF_8);
        println("Instruction bytes within function bodies: "+covered+"; outside: "+uncovered);
        println("Labelled CRT functions: " + labelled + "; mapped functions: " + (rows.size()-1));
    }
}
