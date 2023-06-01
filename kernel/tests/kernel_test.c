#include <cspecs/cspec.h>

context (test_de_estados) {
    char * valor = obtenerEstado(0);
    char * valor2 = obtenerEstado(1);

    describe("Test de lectura de estados") {
        it("El valor 0 deberia ser NEW") {
            should_string(valor) be equal to ("NEW");
        } end
        it("El valor 1 deberia ser READY") {
            should_string(valor2) be equal to ("READY");
        } end
    } end

}
