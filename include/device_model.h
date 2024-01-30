#if !defined(DEVICE_MODEL_H)
#define DEVICE_MODEL_H

#define TTY_IN_N_RN 1 << 0  // 输入时\n转换成\n\r实现换行+回车
#define TTY_IN_DEL  1 << 1
#define TTY_IN_BACKSPACE 1 << 2
#define TTY_OUT_R_N 1 << 10  // 输出时\r转换成\n，实现回车转换成换行
#define TTY_OUT_ECHO 1 << 11
#define TTY_OUT_LINE 1 << 12


#endif // DEVICE_MODEL_H
