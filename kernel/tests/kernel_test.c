#include <../src/kernel.h>
#include <cspecs/cspec.h>

context (test_de_estados) {
    bool la_verdad = true;

    describe("Hello world") {
        it("la_verdad should be true") {
            should_bool(la_verdad) be equal to(true);
        } end
    } end

}
