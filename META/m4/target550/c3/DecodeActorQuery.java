import ghidra.app.script.GhidraScript;
public class DecodeActorQuery extends GhidraScript {
 public void run() throws Exception {
  println(getDataContaining(toAddr(0x4376e0)));
  clearListing(toAddr(0x4376e0),toAddr(0x43770b));
  disassemble(toAddr(0x4376e0));
 }
}
