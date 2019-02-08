# AsmPatchBuilder

This is a simple lightweigth but powerful x86 assembler patch build which can assemble patches for function hooks and detours. 
**It does not do the actual patching** as it only outputs the assembled bytes.

## Simple Example
```cpp
#include <AsmPatchBuilder.h>
#include <cstdio>

int main(int argc, const char* argv[]) {
    AsmPatch::AsmPatchData compiledData = 
        AsmPatch::patch(0x0057FFA4)
            .callLambdaStdcall([this] () { prinf("Hello World!\n") })
			.nop()
			.compile());

    // Now you can use 
    int addr = compiledData.getAddress(); // Equals 0x0057FFA4
    const std::vector<std::uint8_t>& patchData = compiledData.getData();
}
```

## Storing Functions
AsmPatchBuilder will use global thread local storage to store the state of lambda functions.
Therefore do not use this when a patch is compiled multiple times.

## 3rd-party
This header library uses [fixed_size_function](https://github.com/pmed/fixed_size_function) to store the functor as a global variable. 
