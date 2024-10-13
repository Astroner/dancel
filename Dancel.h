#if !defined(DANCEL_H)
#define DANCEL_H

#include <stdint.h>

typedef struct DCLHeader {
    uint16_t id;
    
    uint8_t rd :1; // Recursion Desired
    uint8_t tc :1; // TrunCation
    uint8_t aa :1; // Authoritative Answer
    uint8_t opCode :4; // Kind of query. 0 for standard
    uint8_t qr :1;  // Query or Response

    uint8_t rCode :4; // Response Code
    uint8_t z :3; // Reserved. Should be 0
    uint8_t ra :1; // Recursion Available

    uint16_t qdCount; // Number of Questions
    uint16_t anCount; // Number of Answers
    uint16_t nsCount; // Number of name server resource records in the authority records section
    uint16_t arCount; // Number of resource records in the additional records section
} DCLHeader;

typedef struct DCLQueryHeader {
    uint16_t qType;
    uint16_t qClass;
} DCLQueryHeader;

typedef struct DCLAnswerHeader {
    uint16_t qType;
    uint16_t qClass;
    uint32_t ttl;
    uint16_t rdLength;
} DCLAnswerHeader;

#define DCL_A 1
#define DCL_NS 2
#define DCL_CNAME 4
#define DCL_SOA 8
#define DCL_WKS 16
#define DCL_PTR 32
#define DCL_HINFO 64
#define DCL_MINFO 128
#define DCL_MX 256
#define DCL_TXT 512

typedef struct DCLQuery {
    char* host;
    uint16_t type;
} DCLQuery;

int DCL_writeRequest(uint16_t qID, DCLQuery* queries, unsigned int nQueries, uint8_t* buffer, unsigned int length);

typedef struct DCLParser {
    DCLHeader* headers;
    struct {
        const uint8_t* src;
        const uint8_t* cursor;
        int isQuestion;
        int index;
    } internal;
} DCLParser;

typedef enum DCLElementType {
    DCLQuestion,
    DCLAnswer
} DCLElementType;

typedef union DCLElementValue {
    struct {
        const uint8_t* name;
        int rawNameLength;
        int qType;
        int qClass;
    } question;

    struct {
        const uint8_t* name;
        int rawNameLength;
        int qType;
        int qClass;

        int ttl;
        int dataLength;
        const uint8_t* data;
    } response;
} DCLElementValue;

typedef struct DCLElement {
    DCLElementType type;
    DCLElementValue data;
} DCLElement;

int DCLParser_init(const uint8_t* buffer, DCLParser* parser);
int DCLParser_nextElement(DCLParser* parser, DCLElement* el);
int DCL_extractNameTo(DCLParser* parser, const uint8_t* name, char* buffer, int bufferLength);
int DCL_printPacket(const uint8_t* data);

#endif // DANCEL_H
#if defined(DANCEL_IMPLEMENTATION)


#if !defined(DANCEL_STD_MEMSET)
    #include <string.h>
    #define DANCEL_STD_MEMSET memset
#endif // DANCEL_STD_MEMSET

#if !defined(DANCEL_STD_HTONS)
    #include <arpa/inet.h>
    #define DANCEL_STD_HTONS htons
#endif // DANCEL_STD_HTONS

#if !defined(DANCEL_STD_NTOHS)
    #include <arpa/inet.h>
    #define DANCEL_STD_NTOHS ntohs
#endif // DANCEL_STD_NTOHS

#if !defined(DANCEL_STD_PRINT)
    #include <stdio.h>
    #define DANCEL_STD_PRINT printf
#endif // DANCEL_STD_PRINT


#define DCL_Q_A 1
#define DCL_Q_NS 2
#define DCL_Q_CNAME 5
#define DCL_Q_SOA 6
#define DCL_Q_WKS 11
#define DCL_Q_PTR 12
#define DCL_Q_HINFO 13
#define DCL_Q_MINFO 14
#define DCL_Q_MX 15
#define DCL_Q_TXT 16

#define DCL_CLASS_IN 1 // Internet

static int DCL_internal_writeHost(char* host, uint8_t* dest) {
    char ch;

    uint8_t* labelLength = dest;
    int cursor = 1;
    while((ch = *host++)) {
        if(ch == '.') {
            labelLength = dest + (cursor++);
        } else {
            *labelLength += 1;
            dest[cursor++] = ch;
        }
    }
    dest[cursor] = 0;

    return cursor + 1;
}

int DCL_writeRequest(
    uint16_t qID,
    DCLQuery* queries, 
    unsigned int nQueries, 
    uint8_t* buffer, 
    unsigned int length
) {    
    DANCEL_STD_MEMSET(buffer, 0, length);

    unsigned int cursor = 0;
    DCLHeader* header = (DCLHeader*)buffer;
    cursor += sizeof(*header);


    header->id = DANCEL_STD_HTONS(qID);
    header->rd = 1;

    uint16_t questions = 0;
    
    for(unsigned int i = 0; i < nQueries; i++) {
        DCLQuery* query = queries + i;
        uint16_t hostNameOffset = cursor;
        int written = DCL_internal_writeHost(query->host, buffer + cursor);
        cursor += written;

        DCLQueryHeader* qInfo;
        int hasName = 1;
        if(query->type & DCL_A) {
            questions++;

            hasName = 0;
            qInfo = (DCLQueryHeader*)(buffer + cursor);
            cursor += sizeof(*qInfo);

            qInfo->qType = DANCEL_STD_HTONS(DCL_Q_A);
            qInfo->qClass = DANCEL_STD_HTONS(DCL_CLASS_IN);
        }

        if(query->type & DCL_TXT) {
            questions++;
            if(!hasName) {
                uint16_t* pointer = (uint16_t*)(buffer + cursor);
                cursor += 2;
                *pointer = DANCEL_STD_HTONS(hostNameOffset + 49152); // 49152 = 0b1100000000000000 => pointer mark
            } else {
                hasName = 0;
            }

            qInfo = (DCLQueryHeader*)(buffer + cursor);
            cursor += sizeof(*qInfo);
            
            qInfo->qType = DANCEL_STD_HTONS(DCL_Q_TXT);
            qInfo->qClass = DANCEL_STD_HTONS(DCL_CLASS_IN);
        }

        if(query->type & DCL_MX) {
            questions++;
            if(!hasName) {
                uint16_t* pointer = (uint16_t*)(buffer + cursor);
                cursor += 2;
                *pointer = DANCEL_STD_HTONS(hostNameOffset + 49152); // 49152 = 0b1100000000000000 => pointer mark
            } else {
                hasName = 0;
            }

            qInfo = (DCLQueryHeader*)(buffer + cursor);
            cursor += sizeof(*qInfo);
            
            qInfo->qType = DANCEL_STD_HTONS(DCL_Q_MX);
            qInfo->qClass = DANCEL_STD_HTONS(DCL_CLASS_IN);
        }

        if(query->type & DCL_NS) {
            questions++;
            if(!hasName) {
                uint16_t* pointer = (uint16_t*)(buffer + cursor);
                cursor += 2;
                *pointer = DANCEL_STD_HTONS(hostNameOffset + 49152); // 49152 = 0b1100000000000000 => pointer mark
            } else {
                hasName = 0;
            }

            qInfo = (DCLQueryHeader*)(buffer + cursor);
            cursor += sizeof(*qInfo);
            
            qInfo->qType = DANCEL_STD_HTONS(DCL_Q_NS);
            qInfo->qClass = DANCEL_STD_HTONS(DCL_CLASS_IN);
        }
    }

    header->qdCount = DANCEL_STD_HTONS(questions);

    return cursor;
}


int DCLParser_init(const uint8_t* buffer, DCLParser* parser) {
    parser->headers = (DCLHeader*)buffer;
    parser->headers->id = DANCEL_STD_NTOHS(parser->headers->id);
    parser->headers->opCode = DANCEL_STD_NTOHS(parser->headers->opCode);
    parser->headers->rCode = DANCEL_STD_NTOHS(parser->headers->rCode);

    parser->headers->qdCount = DANCEL_STD_NTOHS(parser->headers->qdCount);
    parser->headers->anCount = DANCEL_STD_NTOHS(parser->headers->anCount);
    parser->headers->nsCount = DANCEL_STD_NTOHS(parser->headers->nsCount);
    parser->headers->arCount = DANCEL_STD_NTOHS(parser->headers->arCount);

    parser->internal.src = buffer;
    parser->internal.cursor = buffer + sizeof(DCLHeader);
    parser->internal.isQuestion = 1;
    parser->internal.index = 0;

    return parser->headers->rCode;
}

static int DCL_internal_getRawNamelength(const uint8_t* name) {
    int length = 0;

    uint8_t ch;
    while((ch = *name++)) {
        if(ch >= 192) { // 0b11000000 - pointer mark
            length += 2;
            return length;
        } else {
            length += 1 + ch;
            name += ch;
        }
    }

    length += 1;

    return length;
}

int DCLParser_nextElement(DCLParser* parser, DCLElement* el) {
    if(parser->internal.isQuestion) {
        el->type = DCLQuestion;

        el->data.question.name = parser->internal.cursor;
        el->data.question.rawNameLength = DCL_internal_getRawNamelength(parser->internal.cursor);
        parser->internal.cursor += el->data.question.rawNameLength;

        el->data.question.qType = DANCEL_STD_NTOHS(*(uint16_t*)parser->internal.cursor);
        el->data.question.qClass = DANCEL_STD_NTOHS(*(uint16_t*)(parser->internal.cursor + 2));
        parser->internal.cursor += 4;

        parser->internal.index += 1;
        if(parser->internal.index == parser->headers->qdCount) {
            parser->internal.isQuestion = 0;
            parser->internal.index = 0;
        }

        return 1;
    }

    if(parser->internal.index == parser->headers->anCount) return 0;
    parser->internal.index += 1;

    el->type = DCLAnswer;
    el->data.response.name = parser->internal.cursor;
    el->data.response.rawNameLength = DCL_internal_getRawNamelength(parser->internal.cursor);
    parser->internal.cursor += el->data.question.rawNameLength;
    
    DCLAnswerHeader* answer = (DCLAnswerHeader*)parser->internal.cursor;
    parser->internal.cursor += sizeof(*answer) - 2; // It should be 10, but due to mem alignment it is 12 so we just subtract 2

    el->data.response.qType = DANCEL_STD_NTOHS(answer->qType);
    el->data.response.qClass = DANCEL_STD_NTOHS(answer->qClass);
    el->data.response.ttl = ntohl(answer->ttl);
    el->data.response.dataLength = DANCEL_STD_NTOHS(answer->rdLength);

    el->data.response.data = parser->internal.cursor;
    parser->internal.cursor += el->data.response.dataLength;

    return 1;
}

int DCL_extractNameTo(DCLParser* parser, const uint8_t* name, char* buffer, int bufferLength) {
    int cursor = 0;

    uint8_t ch;
    while((ch = *name++)) {
        if(cursor > 0) {
            buffer[cursor++] = '.';
            if(cursor == bufferLength - 1) goto finish;
        }
        if(ch >= 192) { // 192 = 0b11000000 - pointer mark
            uint16_t offset = ch * 256 + name[0] - 49152;
            name = parser->internal.src + offset;
        } else {
            for(uint8_t i = 0; i < ch; i++) {
                buffer[cursor++] = name[i];
                if(cursor == bufferLength - 1) goto finish;
            }
            name += ch;
        }
    }

finish:
    buffer[cursor] = '\0';
    return cursor + 1;
}

int DCL_printPacket(const uint8_t* buffer) {
    DCLParser parser;

    DCLParser_init(buffer, &parser);

    DANCEL_STD_PRINT("ID: %d\n", parser.headers->id);
    DANCEL_STD_PRINT("Recursion Desired: %d\n", parser.headers->rd);
    DANCEL_STD_PRINT("TrunCation: %d\n", parser.headers->tc);
    DANCEL_STD_PRINT("Authoritative Answer: %d\n", parser.headers->aa);
    DANCEL_STD_PRINT("OPCODE: %d\n", parser.headers->opCode);
    DANCEL_STD_PRINT("QR: %d\n", parser.headers->qr);

    DANCEL_STD_PRINT("Response Code: %d\n", parser.headers->rCode);
    DANCEL_STD_PRINT("Recursion Available: %d\n", parser.headers->ra);

    DANCEL_STD_PRINT("Number of Questions: %d\n", parser.headers->qdCount);
    DANCEL_STD_PRINT("Number of Answers: %d\n", parser.headers->anCount);
    
    int answersStared = 0;
    int index = 0;
    DANCEL_STD_PRINT("\nQuestions:\n");
    DCLElement el;
    while(DCLParser_nextElement(&parser, &el)) {
        if(el.type == DCLAnswer && !answersStared) {
            answersStared = 1;
            DANCEL_STD_PRINT("\nAnswers:\n");
            index = 0;
        }
        DANCEL_STD_PRINT("[%d]\n", index++);

        char name[100];
        int nameLength = DCL_extractNameTo(&parser, el.data.question.name, name, sizeof(name));
        DANCEL_STD_PRINT("    Name: %s\n", name);
        DANCEL_STD_PRINT("    Name length: %d\n", nameLength);
        DANCEL_STD_PRINT("    Type: %d\n", el.data.question.qType);
        DANCEL_STD_PRINT("    Class: %d\n", el.data.question.qClass);

        if(el.type == DCLQuestion) continue;

        DANCEL_STD_PRINT("    TTL: %d\n", el.data.response.ttl);
        DANCEL_STD_PRINT("    RDLength: %d\n", el.data.response.dataLength);
        DANCEL_STD_PRINT("    RData: ");
        for(int i = 0; i < el.data.response.dataLength; i++) {
            if(el.data.response.qType == 1) {
                if(i != 0) {
                    DANCEL_STD_PRINT(".");
                }
                DANCEL_STD_PRINT("%d", el.data.response.data[i]);
            } else if(el.data.response.qType == 16) {
                DANCEL_STD_PRINT("%c", el.data.response.data[i]);
            } else {
                DANCEL_STD_PRINT("0x%.2X ", el.data.response.data[i]);
            }
        }
        DANCEL_STD_PRINT("\n");
    }

    return 0;
}

#endif // DANCEL_IMPLEMENTATION
