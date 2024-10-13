# Hi there!
This is Dancel - a simple library to write/read DNS packets. **WIP**

# Quick example
```c
#include <stdio.h>

#include DANCEL_IMPLEMENTATION
#include "Dancel.h"

int main(void) {
    // Writing request
    DCLQuery q = {
        .host = "example.com",
        .type = DCL_A
    };

    unsigned char buffer[100];

    DCL_writeRequest(2020, &q, 1, buffer, sizeof(buffer));

    // Reading packet
    DCLParser parser;
    int rCode = DCLParser_init(buffer, &parser);

    printf("rCode: %d\n", rCode);
    printf("Questions: %d\n", parser.headers->qdCount);
    printf("Answers: %d\n", parser.headers->anCount);

    DCLElement el;
    while(DCLParser_nextElement(&parser, &el)) {
        if(el.type == DCLQuestion) {
            printf("Question\n");
        } else {
            printf("Answer\n");
        }
    }

    return 0;
}
```