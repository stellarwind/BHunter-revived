# Source-owned model resource bindings

`bh_render_resources_*` is an explicit source adapter, not a recovered original
function. It binds already recovered `00458140` (`bh_model_load`) to an owned
`BHModelDrawCache`, a per-model `BHModelInstanceContext`, and a tag-0
`BHDrawResource`. The original startup owner `004474c0` loads `PU_9A.BSP` into
global `004d19c4`; `00441a00` selects that global for entity type 9, variant
0, and assigns its `004413b0` draw callback. This module does not claim those
entity or startup functions are restored.

The caller supplies a real `BHModelContext` and an instance prototype with
camera, lighting, clipper, and emitter bindings. The adapter copies the
prototype, replaces model/cache with owned instances, and resets the model
frame epoch, draw status, accumulated counters, and last camera view. Model
arrays and cache working arrays are owned independently per handle. Images
belong to the model context's texture cache and outlive the model owner.

Handles are positive slot-plus-one integers. `bh_render_resources_lookup` has
the `BHDrawResourceLookup` callback signature. Published descriptor pointers
remain stable during table growth; handles remain valid until the owner is
disposed. The queue borrows handles and all render bindings, so the caller
must drain queued commands before disposal. Failed loads do not publish a
handle and release all partial model/cache allocations. No tag-1/2/3
constructor or ownership is inferred from the dispatcher branches.

The focused source-only test loads the real archived `BSP\\PU_9A.BSP`, checks
owned model/cache independence, binding copy, epoch reset, stable lookup
across table growth, invalid handles, allocation failure and complete cleanup.
The model's original asset identity is a fixture for this adapter, not proof
of visual or gameplay fidelity. The original EXE is never executed.

Parent admission: current source additionally requires present camera/config/
clipper/emitter and enabled light bindings, rejects populated-owner reinit, and
passes 25 real-data checks (3 without assets) plus queued native composition in
`META/m4/queued-resource/validation.json`. The earlier focused 22-check result
is historical candidate evidence and is not transferred to this guarded variant.
