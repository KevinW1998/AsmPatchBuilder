# AsmPatchBuilder

This is a simple lightweigth but powerful x86 assembler patch build which can assemble patches for function hooks and detours. 
**It does not do the actual patching** as it only outputs the assembled bytes.

== Simple Example ==
```cpp
#include <AsmPatchBuilder.h>
#include <cstdio>

int main(int argc, const char* argv[]) {
    AsmPatchData compiledData = 
        PATCH(0x0057FFA4)
            .CALL_LAMBDA([this] () { prinf("Hello World!\n") })
			.NOP()
			.Compile());

    // Now you can use 
    int addr = compiledData.getAddress(); // Equals 0x0057FFA4
    const std::vector<std::uint8_t>& patchData = compiledData.getData();
}
```

== Storing Functions ==
AsmPatchBuilder will use global thread local storage to store the state of lambda functions.
Therefore do not use this when a patch is compiled multiple times.
