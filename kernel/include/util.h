#ifndef UTIL_H
#define UTIL_H

#define offsetof(type, member) \
((unsigned long)&(((type *)0)->member))

#define container_of(ptr, type, member) \
((type *)((char *)(ptr) - offsetof(type, member)))

#define ARRAY_SIZE(arr) \
(sizeof(arr) / sizeof((arr)[0]))

#endif
