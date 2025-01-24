#if !defined(NET_NET_ERR_H)
#define NET_NET_ERR_H
typedef enum {
    NET_ERR_NEED_WAIT = 1,
    NET_ERR_OK = 0,
    NET_ERR_PARAM = -1,
    NET_ERR_EXIST = -2,
    NET_ERR_STATE = -3,
    NET_ERR_NONE = -4,
    NET_ERR_SIZE = -5,
    NET_ERR_FULL = -6,
    NET_ERR_MEM = -7,
    NET_ERR_IO = -8,
    NET_ERR_NOT_SUPPORT = -9,
    NET_ERR_NOT_MATCH = -10,
    NET_ERR_CHECKSUM = -11,
    NET_ERR_UNREACH = -12,
    NET_ERR_BINDED = -13,
    NET_ERR_CLOSE = -14,
    NET_ERR_EOF = -15,
} net_err_t;


#endif // NET_NET_ERR_H
