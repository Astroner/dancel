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
