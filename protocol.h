#ifndef PROTOCOL_H_H
#define PROTOCOL_H_H

typedef enum _Protocol {
    PROTOCOL_ALL = 0,   //!< 表示所有协议
    PROTOCOL_HTTP,      //!< 表示http协议
    PROTOCOL_SMTP,      //!< 表示smtp协议
    PROTOCOL_FTP,       //!< 表示ftp协议
} Protocol;

#define MAX_PROTCOL 512

#endif //PROTOCOL_H_H
