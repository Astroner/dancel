#include "tests-new.h"

#define DANCEL_IMPLEMENTATION
#include "../Dancel.h"

#include "tests.h"

DESCRIBE(dancel) {
    IT("works") {
        DCLQuery q = {
            .host = "example.com",
            .type = DCL_A
        };

        unsigned char buffer[100];

        EXPECT(DCL_writeRequest(2020, &q, 1, buffer, sizeof(buffer))) NOT TO_BE(-1);
    }
}